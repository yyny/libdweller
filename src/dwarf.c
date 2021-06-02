/****************************************************************************
 *
 * Copyright 2020 The libdweller project contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
#include <dweller/dwarf.h>
#include <dweller/util.h>
#include <dweller/stream.h>
#include "dwarf_compat.h"
#include "dwarf_error.h"

/* FIXME: remove these unnecessairy dependencies */
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h> /* ffs */

#include "dwarf_symbols.c"
#include "dwarf_abbrev.c"
#include "dwarf_aranges.c"
#include "dwarf_die.c"
#include "dwarf_unit.c"
#include "dwarf_cu.c"
#include "dwarf_error.c"
#include "dwarf_stream.c"
#include "dwarf_read.c"
#include "dwarf_iter.c"

static bool dwarf_parse_aranges_section(struct dwarf *dwarf, struct dwarf_section_aranges *aranges, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(dw_isnull(dwarf))) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!aranges->section.base && !aranges->section_provider)) error(argument_error(2, "aranges", __func__, "no segment data!"));

    dw_stream_t stream = dw_stream_new(DWARF_SECTION_ARANGES, aranges->section, aranges->section_provider);
    while (!dw_stream_isdone(&stream)) {
        dwarf_aranges_t arangelst;
        dwarf_aranges_parseheader(dwarf, &stream, &arangelst, errinfo);
        dw_i64_t off = dw_stream_tell(&stream);
        if (dwarf->aranges_cb) {
            dwarf->errinfo = errinfo;
            dwarf->aranges_cb(dwarf, &arangelst);
        }
        dw_stream_seek(&stream, off);
        dwarf_arange_t arange = { 0, 0, 0 };
        for (;;) {
            dwarf_read_arange(dwarf, &stream, &arangelst, &arange, errinfo);
            if (arange.base == 0 && arange.size == 0) break;
            dw_i64_t off = dw_stream_tell(&stream);
            if (dwarf->arange_cb) {
                dwarf->errinfo = errinfo;
                dwarf->arange_cb(dwarf, &arangelst, &arange);
            }
            if (arangelst.arange_cb) {
                dwarf->errinfo = errinfo;
                arangelst.arange_cb(dwarf, &arangelst, &arange);
            }
            dw_stream_seek(&stream, off);
        }
        dw_stream_get32(&stream); /* FIXME: ??? This is not in spec, why do producers output this? */
        aranges->aranges = dw_realloc(dwarf, aranges->aranges, (aranges->num_aranges + 1) * sizeof(struct dwarf_address_ranges));
        aranges->aranges[aranges->num_aranges++] = arangelst;
    }
    return true;
}
static void append_row(struct dwarf *dwarf, struct dwarf_line_program *program, struct dwarf_line_program_state *state, struct dwarf_line_program_state *last_state, struct dwarf_errinfo *errinfo)
{
    if (dwarf->line_row_cb) {
        dwarf->errinfo = errinfo;
        dwarf->line_row_cb(dwarf, program, state, last_state);
    }
    if (program->line_row_cb) {
        dwarf->errinfo = errinfo;
        program->line_row_cb(dwarf, program, state, last_state);
    }
    *last_state = *state;
}
static void reset_state(struct dwarf *dwarf, struct dwarf_line_program *program, struct dwarf_line_program_state *state)
{
    memset(state, 0x00, sizeof(*state));
    state->file = 1;
    state->line = 1;
    state->is_stmt = program->default_is_stmt;
}
static bool dwarf_parse_line_section_line_program(struct dwarf *dwarf, struct dwarf_section_line *line, struct dwarf_line_program *lineprg, struct dwarf_errinfo *errinfo)
{
    dw_stream_t stream;
    dw_stream_initfrom(&stream, DWARF_SECTION_LINE, line->section, line->section_provider, lineprg->section_offset);
    dw_u32_t length = dw_stream_get32(&stream);
    assert(lineprg->length == length);
    lineprg->version = dw_stream_get16(&stream);
    switch (lineprg->version) {
    case 2:
    case 3:
    case 4: /* TODO: Test version 4 better */
    case 5: /* TODO: Test version 5 better */
        if (lineprg->version >= 5) {
            lineprg->address_size = dw_stream_get8(&stream);
            lineprg->segment_selector_size = dw_stream_get8(&stream);
        }
        lineprg->header_length = dw_stream_get32(&stream); /* TODO: 64 bits in dwarf64 */
        lineprg->instruction_size = dw_stream_get8(&stream);
        if (lineprg->version >= 4) {
            lineprg->maximum_operations_per_instruction = dw_stream_get8(&stream);
        }
        lineprg->default_is_stmt = dw_stream_get8(&stream);
        lineprg->line_base = dw_stream_get8(&stream);
        lineprg->line_range = dw_stream_get8(&stream);
        lineprg->first_special_opcode = dw_stream_get8(&stream);
        uint8_t num_basic_opcodes = lineprg->first_special_opcode - 1;
        lineprg->basic_opcode_argcount = dw_malloc(dwarf, num_basic_opcodes * sizeof(uint8_t));
        size_t i;
        for (i=0; i < num_basic_opcodes; i++) {
            lineprg->basic_opcode_argcount[i] = dw_stream_get8(&stream);
        }
        if (lineprg->version >= 5) {
            /* FIXME: Much of this code is duplicated for version < 5 */
            dwarf_attr_t attr;
            lineprg->directorydata_format_count = dw_stream_get8(&stream);
            lineprg->directorydata_format = dw_malloc(dwarf, lineprg->directorydata_format_count * sizeof(struct dwarf_line_program_format));
            for (i=0; i < lineprg->directorydata_format_count; i++) {
                lineprg->directorydata_format[i].name = dw_stream_getleb128_unsigned(&stream, NULL);
                lineprg->directorydata_format[i].form = dw_stream_getleb128_unsigned(&stream, NULL);
            }
            for (i=0; i < lineprg->directorydata_format_count; i++) {
                attr.name = lineprg->directorydata_format[i].name;
                attr.form = lineprg->directorydata_format[i].form;
                dwarf_read_attr(dwarf, &stream, &attr);
                switch (attr.name) {
                case DW_LNCT_path:
                    switch (attr.form) {
                    case DW_FORM_string:
                    case DW_FORM_strp:
                    case DW_FORM_line_strp:
                        lineprg->include_directories[i].form = attr.form;
                        lineprg->include_directories[i].value = attr.value;
                        break;
                    case DW_FORM_strp_sup:
                        /* TODO */
                        break;
                    default:
                        assert(false); /* TODO */
                        break;
                    }
                    break;
                    break;
                default:
                    break;
                }
            }
            lineprg->num_include_directories = dw_stream_getleb128_unsigned(&stream, NULL);
            lineprg->include_directories = dw_malloc(dwarf, lineprg->num_include_directories * sizeof(dw_str_t));
            for (i=0; i < lineprg->num_include_directories; i++) {
                attr.name = lineprg->directorydata_format[i].name;
                attr.form = lineprg->directorydata_format[i].form;
                dwarf_read_attr(dwarf, &stream, &attr);
            }
            lineprg->filedata_format_count = dw_stream_get8(&stream);
            lineprg->filedata_format = dw_malloc(dwarf, lineprg->filedata_format_count * sizeof(struct dwarf_line_program_format));
            for (i=0; i < lineprg->filedata_format_count; i++) {
                lineprg->directorydata_format[i].name = dw_stream_getleb128_unsigned(&stream, NULL);
                lineprg->directorydata_format[i].form = dw_stream_getleb128_unsigned(&stream, NULL);
            }
            lineprg->num_files = dw_stream_getleb128_unsigned(&stream, NULL);
            lineprg->files = dw_malloc(dwarf, lineprg->num_files * sizeof(struct dwarf_fileinfo));
            for (i=0; i < lineprg->num_files; i++) {
                attr.name = lineprg->directorydata_format[i].name;
                attr.form = lineprg->directorydata_format[i].form;
                dwarf_read_attr(dwarf, &stream, &attr);
                switch (attr.name) {
                case DW_LNCT_path:
                    switch (attr.form) {
                    case DW_FORM_string:
                        lineprg->files[i].name.section = DWARF_SECTION_LINE;
                        lineprg->files[i].name.off = attr.value.str.off;
                        abort(); /* FIXME: Not fully implemented, but really easy */
                        break;
                    case DW_FORM_strp:
                    lineprg->files[i].name.section = DWARF_SECTION_STR;
                        lineprg->files[i].name.off = attr.value.stroff;
                        lineprg->files[i].name.len = -1;
                        break;
                    case DW_FORM_line_strp:
                        lineprg->files[i].name.section = DWARF_SECTION_LINESTR;
                        lineprg->files[i].name.off = attr.value.stroff;
                        lineprg->files[i].name.len = -1;
                        break;
                    case DW_FORM_strp_sup:
                        /* TODO */
                        break;
                    default:
                        assert(false); /* TODO */
                        break;
                    }
                    break;
                case DW_LNCT_directory_index:
                    switch (attr.form) {
                    case DW_FORM_data1:
                    case DW_FORM_data2:
                    case DW_FORM_udata:
                        lineprg->files[i].include_directory_idx = attr.value.val;
                        break;
                    default:
                        assert(false); /* TODO */
                        break;
                    }
                    break;
                case DW_LNCT_timestamp:
                    switch (attr.form) {
                    case DW_FORM_data4:
                    case DW_FORM_data8:
                    case DW_FORM_udata:
                        lineprg->files[i].last_modification_time = attr.value.val;
                        break;
                    case DW_FORM_block:
                        assert(false); /* TODO */
                        break;
                    default:
                        assert(false); /* TODO */
                        break;
                    }
                    break;
                case DW_LNCT_size:
                    switch (attr.form) {
                    case DW_FORM_data1:
                    case DW_FORM_data2:
                    case DW_FORM_data4:
                    case DW_FORM_data8:
                    case DW_FORM_udata:
                        lineprg->files[i].file_size = attr.value.val;
                        break;
                    default:
                        assert(false); /* TODO */
                        break;
                    }
                    break;
                case DW_LNCT_MD5:
                    break;
                default:
                    break;
                }
            }
            /* TODO: call callback for attr? */
        } else {
            lineprg->directorydata_format_count = 0;
            lineprg->directorydata_format = NULL;
            lineprg->filedata_format_count = 0;
            lineprg->filedata_format = NULL;
            lineprg->directorydata = NULL;
            lineprg->filedata = NULL;
            while (dw_stream_peak8(&stream)) {
                dw_i64_t start = dw_stream_tell(&stream);
                struct dwarf_pathinfo path;
                path.form = DW_FORM_string,
                path.value.str.section = DWARF_SECTION_LINE;
                path.value.str.off = start;
                while (dw_stream_get8(&stream));
                path.value.str.len = dw_stream_tell(&stream) - start - 1;
                lineprg->total_include_path_size += path.value.str.len;
                /* TODO: Allocate as OBSTACK */
                lineprg->include_directories = dw_realloc(dwarf, lineprg->include_directories, (lineprg->num_include_directories + 1) * sizeof(struct dwarf_pathinfo));
                lineprg->include_directories[lineprg->num_include_directories++] = path;
            }
            dw_stream_get8(&stream);
            while (dw_stream_peak8(&stream)) {
                struct dwarf_fileinfo info;
                dw_i64_t start = dw_stream_tell(&stream);
                info.name.section = DWARF_SECTION_LINE;
                info.name.off = start;
                while (dw_stream_get8(&stream));
                info.name.len = dw_stream_tell(&stream) - start - 1; /* FIXME: This will be overwritten on the next call to dw_stream!!! */
                lineprg->total_file_path_size += info.name.len;
                info.include_directory_idx = dw_stream_getleb128_unsigned(&stream, NULL);
                info.last_modification_time = dw_stream_getleb128_unsigned(&stream, NULL);
                info.file_size = dw_stream_getleb128_unsigned(&stream, NULL);
                /* TODO: Allocate as OBSTACK */
                lineprg->files = dw_realloc(dwarf, lineprg->files, (lineprg->num_files + 1) * sizeof(struct dwarf_fileinfo));
                lineprg->files[lineprg->num_files++] = info;
            }
            dw_stream_get8(&stream); /* Skip NUL */
        }
        lineprg->header_size = dw_stream_tell(&stream) - lineprg->section_offset;
        if (dwarf->line_cb) {
            dwarf->errinfo = errinfo;
            dwarf->line_cb(dwarf, lineprg);
        }
        dw_stream_isdone(&stream);
        struct dwarf_line_program_state state;
        reset_state(dwarf, lineprg, &state);
        struct dwarf_line_program_state last_state = state;
        last_state.file = 0;
        last_state.line = 0;
        last_state.column = 0;
        while (stream.off < lineprg->section_offset + lineprg->length + sizeof(dw_u32_t)) {
            int basic_opcode = dw_stream_get8(&stream);
            if (basic_opcode >= lineprg->first_special_opcode) { /* This is a special opcode, it takes no arguments */
                int special_opcode = basic_opcode - lineprg->first_special_opcode;
                int line_increment = lineprg->line_base + (special_opcode % lineprg->line_range);
                int address_increment = (special_opcode / lineprg->line_range) * lineprg->instruction_size;
                state.line += line_increment;
                state.address += address_increment;
                dw_i64_t off = dw_stream_tell(&stream);
                append_row(dwarf, lineprg, &state, &last_state, errinfo);
                dw_stream_seek(&stream, off);
                state.basic_block = false;
                state.prologue_end = false;
                state.epilogue_begin = false;
                state.discriminator = false;
            } else if (basic_opcode == DW_LNS_fixed_advance_pc) { /* See docs */
                int address_increment = dw_stream_get16(&stream);
                state.address += address_increment;
            } else if (basic_opcode) { /* This is a basic opcode */
                uint8_t i;
                uint64_t nargs = lineprg->basic_opcode_argcount[basic_opcode - 1]; /* Array is 1-indexed */
                uint64_t *args = dw_malloc(dwarf, nargs * sizeof(uint64_t));
                dw_i64_t argoff = dw_stream_tell(&stream);
                for (i=0; i < nargs; i++) {
                    /* Getting the offsets to arguments other than the first one is
                     * trivial with the second argument to dw_stream_getleb128_unsigned.
                     * We only need to reparse the first one
                     */
                    args[i] = dw_stream_getleb128_unsigned(&stream, NULL);
                }
                switch (basic_opcode) {
                /* Same as basic opcode with `line_increment` and
                 * `address_increment` as zero
                 */
                case DW_LNS_copy:
                    append_row(dwarf, lineprg, &state, &last_state, errinfo);
                    state.basic_block = false;
                    break;
                case DW_LNS_advance_pc:
                    {
                        assert(nargs == 1); /* TODO: Do this test when the basic_opcode_argcount gets filled? */
                        int address_increment = args[0] * lineprg->instruction_size;
                        state.address += address_increment;
                    }
                    break;
                case DW_LNS_advance_line:
                    {
                        dw_i64_t off = dw_stream_tell(&stream);
                        dw_stream_seek(&stream, argoff);
                        state.line += dw_stream_getleb128_signed(&stream, NULL);
                        dw_stream_seek(&stream, off);
                    }
                    break;
                case DW_LNS_set_file:
                    state.file = args[0];
                    break;
                case DW_LNS_set_column:
                    state.column = args[0];
                    break;
                case DW_LNS_negate_stmt:
                    state.is_stmt = !state.is_stmt;
                    break;
                case DW_LNS_set_basic_block:
                    state.basic_block = true;
                    break;
                case DW_LNS_const_add_pc:
                    {
                        int address_increment = ((255 - lineprg->first_special_opcode) / lineprg->line_range) * lineprg->instruction_size;
                        state.address += address_increment;
                    }
                    break;
                case DW_LNS_set_prologue_end:
                    state.prologue_end = true;
                    break;
                case DW_LNS_set_epilogue_begin:
                    state.epilogue_begin = true;
                    break;
                case DW_LNS_set_isa:
                    state.isa = args[0];
                    break;
                case DW_LNS_fixed_advance_pc: /* This is already handled and just to shut up compiler warnings */
                default:
                    break;
                }
                dw_free(dwarf, args);
            } else { /* This is an extended opcode */
                uint64_t i;
                uint64_t extended_opcode_length = dw_stream_getleb128_unsigned(&stream, NULL);
                uint8_t extended_opcode = dw_stream_get8(&stream);
                assert(extended_opcode_length != 0);
                uint8_t *args = dw_malloc(dwarf, extended_opcode_length - 1);
                for (i=0; i < extended_opcode_length - 1; i++) {
                    args[i] = dw_stream_get8(&stream);
                }
                switch (extended_opcode) {
                case DW_LNE_end_sequence:
                    state.end_sequence = true;
                    append_row(dwarf, lineprg, &state, &last_state, errinfo);
                    reset_state(dwarf, lineprg, &state);
                    break;
                case DW_LNE_set_address: /* FIXME: Check that args is actually an address */
                    state.address = *(uint64_t *)args;
                    break;
                case DW_LNE_define_file:
                    /* TODO */
                    break;
                case DW_LNE_set_discriminator:
                    {
                        state.discriminator = dw_leb128_parse_unsigned(args, extended_opcode_length - 1, NULL);
                    }
                    break;
                default:
                    break;
                }
            }
        }
        break;
    default:
        error(runtime_error("unsupported line program version: %1", "I", lineprg->version));
    }
    return true;
}
static bool dwarf_parse_line_section(struct dwarf *dwarf, struct dwarf_section_line *line, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(dw_isnull(dwarf))) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!line->section.base && !line->section_provider)) error(argument_error(2, "line", __func__, "no segment data!"));

    dw_stream_t stream = dw_stream_new(DWARF_SECTION_LINE, line->section, line->section_provider);
    while (!dw_stream_isdone(&stream)) {
        struct dwarf_line_program lineprg;
        memset(&lineprg, 0x00, sizeof(lineprg));
        lineprg.section_offset = stream.off;
        lineprg.length = dw_stream_get32(&stream);
        if (!dwarf_parse_line_section_line_program(dwarf, line, &lineprg, errinfo)) return false;
        dw_stream_offset(&stream, lineprg.length);
    }
    return true;
}
static bool dwarf_parse_abbrev_section(struct dwarf *dwarf, struct dwarf_section_abbrev *abbrev, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!abbrev->section.base && !abbrev->section_provider)) error(argument_error(2, "abbrev", __func__, "no segment data!"));
    dw_stream_t stream = dw_stream_new(DWARF_SECTION_ABBREV, abbrev->section, abbrev->section_provider);
    while (!dw_stream_isdone(&stream)) {
        dwarf_abbrev_table_t table;
        memset(&table, 0x00, sizeof(table));
        table.debug_abbrev_offset = stream.off;
        table.is_sequential = true;
        while (dw_stream_peak8(&stream)) {
            dwarf_abbrev_t abbrev;
            memset(&abbrev, 0x00, sizeof(abbrev));
            abbrev.offset = stream.off;
            abbrev.abbrev_code = dw_stream_getleb128_unsigned(&stream, NULL);
            abbrev.tag = dw_stream_getleb128_unsigned(&stream, NULL);
            abbrev.has_children = dw_stream_get8(&stream);
            if (abbrev.abbrev_code != table.num_abbreviations + 1) {
                table.is_sequential = false;
            }
            dw_i64_t off = dw_stream_tell(&stream);
            if (dwarf->abbrev_cb) {
                dwarf->errinfo = errinfo;
                dwarf->abbrev_cb(dwarf, &abbrev);
            }
            dw_stream_seek(&stream, off);
            while (dw_stream_peak8(&stream)) {
                dwarf_abbrev_attr_t abbrev_attr;
                abbrev_attr.name = dw_stream_getleb128_unsigned(&stream, NULL);
                abbrev_attr.form = dw_stream_getleb128_unsigned(&stream, NULL);
                dw_i64_t off = dw_stream_tell(&stream);
                if (dwarf->abbrev_attr_cb) {
                    dwarf->errinfo = errinfo;
                    dwarf->abbrev_attr_cb(dwarf, &abbrev, &abbrev_attr);
                }
                if (abbrev.abbrev_attr_cb) {
                    dwarf->errinfo = errinfo;
                    abbrev.abbrev_attr_cb(dwarf, &abbrev, &abbrev_attr);
                }
                dw_stream_seek(&stream, off);
            }
            dw_stream_get8(&stream); /* Null attribute ID */
            dw_stream_get8(&stream); /* Null form ID */
            table.abbreviations = dw_realloc(dwarf, table.abbreviations, (table.num_abbreviations + 1) * sizeof(dwarf_abbrev_t));
            table.abbreviations[table.num_abbreviations++] = abbrev;
        }
        while (!dw_stream_peak8(&stream) && !dw_stream_isdone(&stream)) dw_stream_get8(&stream); /* Null entry to end table, also accounts for potential padding */
        abbrev->tables = dw_realloc(dwarf, abbrev->tables, (abbrev->num_tables + 1) * sizeof(dwarf_abbrev_table_t));
        abbrev->tables[abbrev->num_tables++] = table;
    }
    return true;
}
bool dwarf_parse_die(struct dwarf *dwarf, dwarf_unit_t *unit, dw_stream_t *stream, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    struct dwarf_abbreviation_table *abtable = dwarf_find_abbreviation_table_at_offset(dwarf, unit->debug_abbrev_offset);
    if (!abtable) error(runtime_error("couldn't find an abbreviation table at offset %1 (for compilation unit at offset %2)", "II", unit->debug_abbrev_offset, unit->die.section_offset));

    int depth = 0;
    enum dw_cb_status attr_cb_status = DW_CB_OK;
    do {
        if (!dw_stream_peak8(stream)) {
            if (depth == 0) {
                /* Empty compilation unit */
                /* TODO: Should this be an error? */
                break;
            }
            dw_stream_get8(stream);
            depth--;
            continue;
        }
        dwarf_die_t die;
        if (!dwarf_die_init(dwarf, &die, errinfo)) return false;
        die.section = unit->die.section;
        die.section_offset = dw_stream_tell(stream);
        die.abbrev_code = dw_stream_getleb128_unsigned(stream, NULL);
        die.depth = depth;
        dwarf_abbrev_t *abbrev = dwarf_abbrev_table_find_abbrev_from_code(dwarf, abtable, die.abbrev_code);
        assert(abbrev); /* FIXME */
        dw_stream_t abstream;
        dw_stream_initfrom(&abstream, DWARF_SECTION_ABBREV, dwarf->abbrev.section, dwarf->abbrev.section_provider, abbrev->offset);
        /* Skip over the abbreviation code */
        check(dw_stream_getleb128_unsigned(&abstream, NULL) == die.abbrev_code);
        die.tag = dw_stream_getleb128_unsigned(&abstream, NULL);
        die.has_children = dw_stream_get8(&abstream);
        dw_i64_t off = dw_stream_tell(stream);
        dw_i64_t aboff = dw_stream_tell(&abstream);
        if (dwarf->die_cb) {
            dwarf->errinfo = errinfo;
            dwarf->die_cb(dwarf, unit, &die);
        }
        if (unit->die_cb) {
            dwarf->errinfo = errinfo;
            unit->die_cb(dwarf, unit, &die);
        }
        if (die.has_children) depth++;
        attr_again: // TODO: Code duplication (Search attr_again)
        dw_stream_seek(stream, off);
        dw_stream_seek(&abstream, aboff);
        dwarf_attr_t attr;
        while (dw_stream_peak16(&abstream)) {
            attr.name = dw_stream_getleb128_unsigned(&abstream, NULL);
            attr.form = dw_stream_getleb128_unsigned(&abstream, NULL);
            dwarf_read_attr(dwarf, stream, &attr);
            dw_i64_t off = dw_stream_tell(stream);
            if (dwarf->attr_cb && attr_cb_status != DW_CB_DONE) {
                dwarf->errinfo = errinfo;
                attr_cb_status = dwarf->attr_cb(dwarf, unit, &die, &attr);
                if (attr_cb_status == DW_CB_RESTART) goto attr_again;
            }
            if (die.attr_cb) {
                dwarf->errinfo = errinfo;
                enum dw_cb_status status = die.attr_cb(dwarf, unit, &die, &attr);
                if (status == DW_CB_DONE) die.attr_cb = NULL;
                if (status == DW_CB_RESTART) goto attr_again;
            }
            dw_stream_seek(stream, off);
        }
        attr.name = 0;
        attr.form = 0;
        if (dwarf->attr_cb && attr_cb_status != DW_CB_DONE) {
            dwarf->errinfo = errinfo;
            attr_cb_status = dwarf->attr_cb(dwarf, unit, &die, &attr);
            if (attr_cb_status == DW_CB_RESTART) goto attr_again;
        }
        if (die.attr_cb) {
            dwarf->errinfo = errinfo;
            enum dw_cb_status status = die.attr_cb(dwarf, unit, &die, &attr);
            if (status == DW_CB_DONE) die.attr_cb = NULL;
            if (status == DW_CB_RESTART) goto attr_again;
        }
        dw_stream_get16(&abstream); /* Skip the null entry */
    } while (depth);

    return true;
}
bool dwarf_parse_die_at(struct dwarf *dwarf, dwarf_unit_t *unit, dw_i64_t *poff, struct dwarf_errinfo *errinfo) {
    dw_stream_t stream;
    dw_stream_initfrom(&stream, DWARF_SECTION_INFO, dwarf->info.section, dwarf->info.section_provider, *poff);

    return dwarf_parse_die(dwarf, unit, &stream, errinfo);
}

DWFUN(bool) dwarf_unit_at(struct dwarf *dwarf, dwarf_unit_t *unit, dw_u64_t off, struct dwarf_errinfo *errinfo)
{
    if (!dwarf_unit_init(dwarf, unit, errinfo)) return false;
    unit->die.section = DWARF_SECTION_INFO;
    dw_stream_t stream;
    dw_stream_initfrom(&stream, DWARF_SECTION_INFO, dwarf->info.section, dwarf->info.section_provider, off);
    return dwarf_unit_parseheader(dwarf, &stream, unit, errinfo);
}
DWFUN(bool) dwarf_die_at(struct dwarf *dwarf, dwarf_unit_t *unit, dwarf_die_t *die, dw_u64_t off, struct dwarf_errinfo *errinfo)
{
    if (!dwarf_die_init(dwarf, die, errinfo)) return false;
    die->section = unit->die.section;
    die->section_offset = unit->die.section_offset + off;
    die->depth = -1;
    dw_stream_t stream;
    dw_stream_initfrom(&stream, DWARF_SECTION_INFO, dwarf->info.section, dwarf->info.section_provider, die->section_offset);
    die->abbrev_code = dw_stream_getleb128_unsigned(&stream, NULL);
    dwarf_abbrev_t *abbrev = dwarf_abbrev_table_find_abbrev_from_code(dwarf, unit->abbrev_table, die->abbrev_code);
    assert(abbrev); /* FIXME */
    dw_stream_t abstream;
    dw_stream_initfrom(&abstream, DWARF_SECTION_ABBREV, dwarf->abbrev.section, dwarf->abbrev.section_provider, abbrev->offset);
    /* Skip over the abbreviation code */
    check(dw_stream_getleb128_unsigned(&abstream, NULL) == die->abbrev_code);
    die->tag = dw_stream_getleb128_unsigned(&abstream, NULL);
    die->has_children = dw_stream_get8(&abstream);
    return true;
}

static bool dwarf_parse_compilation_unit_from_abreviation_table(struct dwarf *dwarf, struct dwarf_compilation_unit *cu, struct dwarf_abbreviation_table *abtable, dw_stream_t *stream, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    int depth = 0;
    enum dw_cb_status attr_cb_status = DW_CB_OK;
    do {
        if (!dw_stream_peak8(stream)) {
            if (depth == 0) {
                /* Empty compilation unit */
                /* TODO: Should this be an error? */
                break;
            }
            dw_stream_get8(stream);
            depth--;
            continue;
        }
        dwarf_die_t die;
        if (!dwarf_die_init(dwarf, &die, errinfo)) return false;
        die.section = cu->unit.die.section;
        die.section_offset = dw_stream_tell(stream);
        die.abbrev_code = dw_stream_getleb128_unsigned(stream, NULL);
        die.depth = depth;
        dwarf_abbrev_t *abbrev = dwarf_abbrev_table_find_abbrev_from_code(dwarf, abtable, die.abbrev_code);
        assert(abbrev); /* FIXME */
        dw_stream_t abstream;
        dw_stream_initfrom(&abstream, DWARF_SECTION_ABBREV, dwarf->abbrev.section, dwarf->abbrev.section_provider, abbrev->offset);
        /* Skip over the abbreviation code */
        check(dw_stream_getleb128_unsigned(&abstream, NULL) == die.abbrev_code);
        die.tag = dw_stream_getleb128_unsigned(&abstream, NULL);
        die.has_children = dw_stream_get8(&abstream);
        dw_i64_t off = dw_stream_tell(stream);
        dw_i64_t aboff = dw_stream_tell(&abstream);
        if (dwarf->die_cb) {
            dwarf->errinfo = errinfo;
            dwarf->die_cb(dwarf, &cu->unit, &die);
        }
        if (cu->unit.die_cb) {
            dwarf->errinfo = errinfo;
            cu->unit.die_cb(dwarf, &cu->unit, &die);
        }
        if (die.has_children) depth++;
        attr_again: // TODO: Code duplication (Search attr_again)
        dw_stream_seek(stream, off);
        dw_stream_seek(&abstream, aboff);
        dwarf_attr_t attr;
        while (dw_stream_peak16(&abstream)) {
            attr.name = dw_stream_getleb128_unsigned(&abstream, NULL);
            attr.form = dw_stream_getleb128_unsigned(&abstream, NULL);
            dwarf_read_attr(dwarf, stream, &attr);
            dw_i64_t off = dw_stream_tell(stream);
            if (dwarf->attr_cb && attr_cb_status != DW_CB_DONE) {
                dwarf->errinfo = errinfo;
                attr_cb_status = dwarf->attr_cb(dwarf, &cu->unit, &die, &attr);
                if (attr_cb_status == DW_CB_RESTART) goto attr_again;
            }
            if (die.attr_cb) {
                dwarf->errinfo = errinfo;
                enum dw_cb_status status = die.attr_cb(dwarf, &cu->unit, &die, &attr);
                if (status == DW_CB_DONE) die.attr_cb = NULL;
                if (status == DW_CB_RESTART) goto attr_again;
            }
            dw_stream_seek(stream, off);
        }
        attr.name = 0;
        attr.form = 0;
        if (dwarf->attr_cb && attr_cb_status != DW_CB_DONE) {
            dwarf->errinfo = errinfo;
            attr_cb_status = dwarf->attr_cb(dwarf, &cu->unit, &die, &attr);
            if (attr_cb_status == DW_CB_RESTART) goto attr_again;
        }
        if (die.attr_cb) {
            dwarf->errinfo = errinfo;
            enum dw_cb_status status = die.attr_cb(dwarf, &cu->unit, &die, &attr);
            if (status == DW_CB_DONE) die.attr_cb = NULL;
            if (status == DW_CB_RESTART) goto attr_again;
        }
        dw_stream_get16(&abstream); /* Skip the null entry */
    } while (depth);

    return true;
}
static bool dwarf_parse_info_section_cu(struct dwarf *dwarf, struct dwarf_section_info *info, dwarf_cu_t *cu, struct dwarf_errinfo *errinfo)
{
    dw_stream_t stream;
    dw_stream_initfrom(&stream, DWARF_SECTION_INFO, info->section, info->section_provider, cu->unit.die.section_offset);
    dwarf_unit_parseheader(dwarf, &stream, &cu->unit, errinfo); // TODO: Error handling
    return dwarf_parse_compilation_unit_from_abreviation_table(dwarf, cu, cu->unit.abbrev_table, &stream, errinfo);
}
static bool dwarf_parse_info_section(struct dwarf *dwarf, struct dwarf_section_info *info, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(dw_isnull(dwarf))) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!info->section.base && !info->section_provider)) error(argument_error(2, "info", __func__, "no segment data!"));

    dw_stream_t stream = dw_stream_new(DWARF_SECTION_INFO, info->section, info->section_provider);
    while (!dw_stream_isdone(&stream)) {
        dwarf_cu_t cu;
        if (!dwarf_cu_init(dwarf, &cu, errinfo)) return false;
        cu.unit.die.section = DWARF_SECTION_INFO;
        cu.unit.die.section_offset = stream.off;
        cu.unit.die.parent = NULL;
        cu.unit.die.depth = 0;
        cu.unit.die.length = dw_stream_get32(&stream);
        dw_i64_t off = dw_stream_tell(&stream);
        enum dw_cb_status cu_cb_status = DW_CB_OK;
        if (dwarf->cu_cb) {
            dwarf->errinfo = errinfo;
            cu_cb_status = dwarf->cu_cb(dwarf, &cu);
            if (cu_cb_status == DW_CB_DONE) dwarf->cu_cb = NULL;
            if (cu_cb_status == DW_CB_RESTART) {
                dw_stream_seek(&stream, 0);
                continue;
            }
        }
        if (cu_cb_status != DW_CB_NEXT) {
            dwarf_parse_info_section_cu(dwarf, info, &cu, errinfo);
        }
        dw_stream_seek(&stream, off);
        dw_stream_offset(&stream, cu.unit.die.length);
    }
    return true;
}
bool dwarf_parse_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(dw_isnull(dwarf))) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    switch (ns) {
    case DWARF_SECTION_ARANGES:
        if (!dwarf_parse_aranges_section(dwarf, &dwarf->aranges, errinfo)) return false;
        break;
    case DWARF_SECTION_INFO:
        if (!dwarf_parse_info_section(dwarf, &dwarf->info, errinfo)) return false;
        break;
    case DWARF_SECTION_LINE:
        if (!dwarf_parse_line_section(dwarf, &dwarf->line, errinfo)) return false;
        break;
    case DWARF_SECTION_ABBREV:
        if (!dwarf_parse_abbrev_section(dwarf, &dwarf->abbrev, errinfo)) return false;
        break;
    case DWARF_SECTION_STR:
        /* FIXME: Nothing to parse? */
        break;
    /* TODO: More sections... */
    }

    return true;
}
bool dwarf_parse(struct dwarf *dwarf, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(dw_isnull(dwarf))) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    if (!dwarf_parse_section(dwarf, DWARF_SECTION_ARANGES, errinfo)) return false;
    if (!dwarf_parse_section(dwarf, DWARF_SECTION_ABBREV, errinfo)) return false;
    if (!dwarf_parse_section(dwarf, DWARF_SECTION_LINE, errinfo)) return false;
    if (!dwarf_parse_section(dwarf, DWARF_SECTION_INFO, errinfo)) return false;
    if (!dwarf_parse_section(dwarf, DWARF_SECTION_STR, errinfo)) return false;

    return true;
}

bool dwarf_walk_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_errinfo *errinfo)
{
    return dwarf_parse_section(dwarf, ns, errinfo);
}
bool dwarf_walk(struct dwarf *dwarf, struct dwarf_errinfo *errinfo)
{
    return dwarf_parse(dwarf, errinfo);
}

bool dwarf_load_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_section section, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(dw_isnull(dwarf))) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    switch (ns) {
    case DWARF_SECTION_ARANGES:
        dwarf->aranges.section = section;
        dwarf->aranges.section_provider = NULL;
        break;
    case DWARF_SECTION_INFO:
        dwarf->info.section = section;
        dwarf->info.section_provider = NULL;
        break;
    case DWARF_SECTION_LINE:
        dwarf->line.section = section;
        dwarf->line.section_provider = NULL;
        break;
    case DWARF_SECTION_ABBREV:
        dwarf->abbrev.section = section;
        dwarf->abbrev.section_provider = NULL;
        break;
    case DWARF_SECTION_STR:
        dwarf->str.section = section;
        dwarf->str.section_provider = NULL;
        break;
    }

    return true;
}
bool dwarf_add_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_section_provider *provider, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(dw_isnull(dwarf))) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    switch (ns) {
    case DWARF_SECTION_ARANGES:
        dwarf->aranges.section_provider = provider;
        break;
    case DWARF_SECTION_INFO:
        dwarf->info.section_provider = provider;
        break;
    case DWARF_SECTION_LINE:
        dwarf->line.section_provider = provider;
        break;
    case DWARF_SECTION_ABBREV:
        dwarf->abbrev.section_provider = provider;
        break;
    case DWARF_SECTION_STR:
        dwarf->str.section_provider = provider;
        break;
    }

    return true;
}

bool dwarf_init(struct dwarf **dwarf, dw_alloc_t *allocator, struct dwarf_errinfo *errinfo)
{
    struct dwarf_alloc_req allocation_request = {
        sizeof(struct dwarf),
        DWARF_ALLOC_STATIC,
        alignof(struct dwarf),
        sizeof(struct dwarf)
    };
    memset(errinfo, 0x00, sizeof(struct dwarf_errinfo));
    if (dw_unlikely(dw_isnull(dwarf))) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!allocator)) error(argument_error(2, "allocator", __func__, "must provide an allocator"));

    *dwarf = NULL;
    int status = (*allocator)(allocator, &allocation_request, (void **)dwarf);
    if (dw_unlikely(status < 0 || dw_isnull(dwarf))) error(allocator_error(allocator, sizeof(struct dwarf), *dwarf, "failed to allocate dwarf state"));
    memset(*dwarf, 0x00, sizeof(struct dwarf));
    (*dwarf)->allocator = allocator;

    return true;
}
void dwarf_fini(struct dwarf **dwarf, const struct dwarf_errinfo *errinfo)
{
    static struct dwarf_alloc_req deallocation_request = {
        0,
        DWARF_ALLOC_STATIC,
        alignof(struct dwarf),
        sizeof(struct dwarf)
    };

    /* Don't need to return errors since we are in the finalizer */
    if (dw_unlikely(dw_isnull(dwarf))) return; /* You're a weirdo */
    if (dw_unlikely(!*dwarf)) return; /* We already freed it */

    dw_alloc_t *allocator = (*dwarf)->allocator;
    if (allocator) {
        DW_USE((*allocator)(allocator, &deallocation_request, (void **)dwarf));
        /* ^ Ignore status for deallocation call */
        /* Give allocator a chance to free internal state */
        DW_USE((*allocator)(allocator, NULL, NULL));
    }
    *dwarf = NULL; /* Ensure it is `NULL` */
}
#pragma GCC diagnostic pop
