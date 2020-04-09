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
#include <dweller/dwarf.h>
#include <dweller/util.h>
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
#include "dwarf_parser.c"
#include "dwarf_die.c"
#include "dwarf_cu.c"
#include "dwarf_error.c"

static bool dwarf_parse_aranges_section(struct dwarf *dwarf, struct dwarf_section_aranges *aranges, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!aranges->section.base)) error(argument_error(2, "aranges", __func__, "no segment data!"));

    struct Parser parser;
    parser.base = aranges->section.base;
    parser.size = aranges->section.size;
    parser.cur = parser.base;
    while (!done(&parser)) {
        dwarf_aranges_t arangelst;
        memset(&arangelst, 0x00, sizeof(arangelst));
        arangelst.section_offset = parser.cur - parser.base;
        arangelst.length = get32(&parser);
        arangelst.version = get16(&parser);
        assert(arangelst.version == 2); /* FIXME: implement version 1 too, and error on 3+ */
        arangelst.debug_info_offset = get32(&parser);
        arangelst.address_size = get8(&parser);
        arangelst.segment_size = get8(&parser);
        assert(arangelst.segment_size == 0);
        assert(arangelst.address_size == 8);
        if (dwarf->aranges_cb) {
            dwarf->aranges_cb(dwarf, &arangelst);
        }
        dwarf_arange_t arange = { 0, 0, 0 };
        do {
            int i;
            arange.segment = 0;
            arange.base = 0;
            arange.size = 0;
            for (i=0; i < arangelst.segment_size; i++) { get8(&parser); }
            arange.base = *(uint32_t *)(parser.cur + sizeof(uint32_t)); /* FIXME: Test if DWARF-64 */
            for (i=0; i < arangelst.address_size; i++) { get8(&parser); }
            arange.size = *(uint32_t *)(parser.cur + sizeof(uint32_t));
            for (i=0; i < arangelst.address_size; i++) { get8(&parser); }
            if (arange.base) assert(arange.size);
            if (dwarf->arange_cb) {
                dwarf->arange_cb(dwarf, &arangelst, &arange);
            }
            if (arangelst.arange_cb) {
                arangelst.arange_cb(dwarf, &arangelst, &arange);
            }
        } while (arange.base || arange.size);
        get32(&parser); /* FIXME: ??? */
        aranges->aranges = dw_realloc(dwarf, aranges->aranges, (aranges->num_aranges + 1) * sizeof(struct dwarf_address_ranges));
        aranges->aranges[aranges->num_aranges++] = arangelst;
    }
    return true;
}
static void append_row(struct dwarf *dwarf, struct dwarf_line_program *program, struct dwarf_line_program_state *state, struct dwarf_line_program_state *last_state)
{
    if (dwarf->line_row_cb) {
        dwarf->line_row_cb(dwarf, program, state, last_state);
    }
    if (program->line_row_cb) {
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
    struct Parser parser;
    parser.base = line->section.base + lineprg->section_offset;
    parser.size = line->section.size - lineprg->section_offset;
    parser.cur = parser.base;
    dw_u32_t length = get32(&parser);
    assert(lineprg->length == length);
    lineprg->version = get16(&parser);
    assert(lineprg->version == 2);
    lineprg->header_length = get32(&parser);
    lineprg->instruction_size = get8(&parser);
    lineprg->default_is_stmt = get8(&parser);
    lineprg->line_base = get8(&parser);
    lineprg->line_range = get8(&parser);
    lineprg->num_basic_opcodes = get8(&parser);
    lineprg->basic_opcode_argcount = dw_malloc(dwarf, lineprg->num_basic_opcodes * sizeof(uint8_t));
    uint8_t i;
    for (i=0; i < lineprg->num_basic_opcodes - 1; i++) {
        lineprg->basic_opcode_argcount[i] = get8(&parser);
    }
    while (peak8(&parser)) {
        dw_str_t path = parser.cur - parser.base;
        while (get8(&parser));
        /* TODO: Allocate as OBSTACK */
        lineprg->include_directories = dw_realloc(dwarf, lineprg->include_directories, (lineprg->num_include_directories + 1) * sizeof(dw_str_t));
        lineprg->include_directories[lineprg->num_include_directories++] = path;
    }
    get8(&parser);
    while (peak8(&parser)) {
        struct dwarf_fileinfo info;
        info.name = parser.cur - parser.base;
        while (get8(&parser));
        info.include_directory_idx = getvar64(&parser, NULL);
        info.last_modification_time = getvar64(&parser, NULL);
        info.file_size = getvar64(&parser, NULL);
        /* TODO: Allocate as OBSTACK */
        lineprg->files = dw_realloc(dwarf, lineprg->files, (lineprg->num_files + 1) * sizeof(struct dwarf_fileinfo));
        lineprg->files[lineprg->num_files++] = info;
    }
    get8(&parser); /* Skip NUL */
    if (dwarf->line_cb) {
        dwarf->line_cb(dwarf, lineprg);
    }
    struct dwarf_line_program_state state;
    reset_state(dwarf, lineprg, &state);
    struct dwarf_line_program_state last_state = state;
    last_state.file = 0;
    last_state.line = 0;
    last_state.column = 0;
    while ((parser.cur - parser.base) < lineprg->length + sizeof(dw_u32_t)) {
        int basic_opcode = get8(&parser);
        if (basic_opcode > lineprg->num_basic_opcodes) { /* This is a special opcode, it takes no arguments */
            int special_opcode = basic_opcode - lineprg->num_basic_opcodes;
            int line_increment = lineprg->line_base + (special_opcode % lineprg->line_range);
            int address_increment = (special_opcode / lineprg->line_range) * lineprg->instruction_size;
            state.line += line_increment;
            state.address += address_increment;
            append_row(dwarf, lineprg, &state, &last_state);
            state.basic_block = false;
            state.prologue_end = false;
            state.epilogue_begin = false;
            state.discriminator = false;
        } else if (basic_opcode == DW_LNS_fixed_advance_pc) { /* See docs */
            int address_increment = get16(&parser);
        } else if (basic_opcode) { /* This is a basic opcode */
            uint8_t i;
            const char *name = dwarf_get_symbol_name(DW_LNS, basic_opcode);
            uint64_t nargs = lineprg->basic_opcode_argcount[basic_opcode - 1]; /* Array is 1-indexed */
            uint64_t *args = dw_malloc(dwarf, nargs * sizeof(uint64_t));
            for (i=0; i < nargs; i++) {
                args[i] = getvar64(&parser, NULL);
            }
            switch (basic_opcode) {
            /* Same as basic opcode with `line_increment` and
             * `address_increment` as zero
             */
            case DW_LNS_copy:
                append_row(dwarf, lineprg, &state, &last_state);
                state.basic_block = false;
                break;
            case DW_LNS_advance_pc:
                {
                    assert(nargs == 1); /* TODO: Do this test when the basic_opcode_argcount gets filled */
                    int address_increment = args[0] * lineprg->instruction_size;
                    state.address += address_increment;
                }
                break;
            case DW_LNS_advance_line:
                state.line += var64_tosigned(args[0]);
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
                    int address_increment = ((255 - lineprg->num_basic_opcodes) / lineprg->line_range) * lineprg->instruction_size;
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
        } else { /* This is an extended opcode */
            uint64_t i;
            uint64_t extended_opcode_length = getvar64(&parser, NULL);
            uint8_t extended_opcode = get8(&parser);
            const char *extended_opcode_name = dwarf_get_symbol_name(DW_LNE, extended_opcode);
            assert(extended_opcode_length != 0);
            uint8_t *args = dw_malloc(dwarf, extended_opcode_length - 1);
            for (i=0; i < extended_opcode_length - 1; i++) {
                args[i] = get8(&parser);
            }
            switch (extended_opcode) {
            case DW_LNE_end_sequence:
                state.end_sequence = true;
                append_row(dwarf, lineprg, &state, &last_state);
                reset_state(dwarf, lineprg, &state);
                break;
            case DW_LNE_set_address: /* FIXME: Check that args is actually an address */
                state.address = *(uint64_t *)args;
                break;
            case DW_LNE_set_discriminator:
                {
                    struct Parser parser;
                    parser.base = args;
                    parser.size = extended_opcode_length - 1;
                    parser.cur = parser.base;
                    state.discriminator = getvar64(&parser, NULL);
                }
                break;
            default:
                break;
            }
        }
    }
    return true;
}
static bool dwarf_parse_line_section(struct dwarf *dwarf, struct dwarf_section_line *line, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!line->section.base)) error(argument_error(2, "line", __func__, "no segment data!"));

    struct Parser parser;
    parser.base = line->section.base;
    parser.size = line->section.size;
    parser.cur = parser.base;
    while (!done(&parser)) {
        struct dwarf_line_program lineprg;
        memset(&lineprg, 0x00, sizeof(lineprg));
        lineprg.section_offset = parser.cur - parser.base;
        lineprg.length = get32(&parser);
        if (!dwarf_parse_line_section_line_program(dwarf, line, &lineprg, errinfo)) return false;
        parser.cur += lineprg.length;
    }
    return true;
}
dwarf_abbrev_t *dwarf_abbrev_table_find_abbrev_from_code(struct dwarf *dwarf, struct dwarf_abbreviation_table *table, dw_symval_t abbrev_code)
{
    if (abbrev_code == 0) return NULL;
    if (abbrev_code > table->num_abbreviations) return NULL;
    if (table->is_sequential) return &table->abbreviations[abbrev_code - 1];
    else {
        /*
        TODO: bsearch if sorted, raw index if linear
        */
        size_t i;
        for (i=0; i < table->num_abbreviations; i++) {
            dwarf_abbrev_t *abbrev = &table->abbreviations[i];
            if (abbrev->abbrev_code == abbrev_code) {
                return abbrev;
            }
        }
    }
    return NULL;
}
static bool dwarf_parse_abbrev_section(struct dwarf *dwarf, struct dwarf_section_abbrev *abbrev, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!abbrev->section.base)) error(argument_error(2, "abbrev", __func__, "no segment data!"));
    struct Parser parser;
    parser.base = abbrev->section.base;
    parser.size = abbrev->section.size;
    parser.cur = parser.base;
    while (!done(&parser)) {
        dwarf_abbrev_table_t table;
        memset(&table, 0x00, sizeof(table));
        table.debug_abbrev_offset = parser.cur - parser.base;
        table.is_sequential = true;
        while (peak8(&parser)) {
            dwarf_abbrev_t abbrev;
            memset(&abbrev, 0x00, sizeof(abbrev));
            abbrev.offset = parser.cur - parser.base;
            abbrev.abbrev_code = getvar64(&parser, NULL);
            abbrev.tag = getvar64(&parser, NULL);
            abbrev.has_children = get8(&parser);
            if (abbrev.abbrev_code != table.num_abbreviations + 1) {
                table.is_sequential = false;
            }
            if (dwarf->abbrev_cb) {
                dwarf->abbrev_cb(dwarf, &abbrev);
            }
            while (peak8(&parser)) {
                dwarf_abbrev_attr_t abbrev_attr;
                abbrev_attr.name = getvar64(&parser, NULL);
                abbrev_attr.form = getvar64(&parser, NULL);
                if (dwarf->abbrev_attr_cb) {
                    dwarf->abbrev_attr_cb(dwarf, &abbrev, &abbrev_attr);
                }
                if (abbrev.abbrev_attr_cb) {
                    abbrev.abbrev_attr_cb(dwarf, &abbrev, &abbrev_attr);
                }
            }
            get8(&parser); /* Null attribute ID */
            get8(&parser); /* Null form ID */
            table.abbreviations = dw_realloc(dwarf, table.abbreviations, (table.num_abbreviations + 1) * sizeof(dwarf_abbrev_t));
            table.abbreviations[table.num_abbreviations++] = abbrev;
        }
        while (!peak8(&parser) && !done(&parser)) get8(&parser); /* Null entry to end table, also accounts for potential padding */
        abbrev->tables = dw_realloc(dwarf, abbrev->tables, (abbrev->num_tables + 1) * sizeof(dwarf_abbrev_table_t));
        abbrev->tables[abbrev->num_tables++] = table;
    }
    return true;
}
static bool dwarf_parse_compilation_unit_from_abreviation_table(struct dwarf *dwarf, struct dwarf_compilation_unit *cu, struct dwarf_abbreviation_table *abtable, struct Parser *parser, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    int depth = 0;
    enum dw_cb_status attr_cb_status = DW_CB_OK;
    do {
        if (!peak8(parser)) {
            if (depth == 0) {
                /* Empty compilation unit */
                /* TODO: Should this be an error? */
                break;
            }
            get8(parser);
            depth--;
            continue;
        }
        dwarf_die_t die;
        if (!dwarf_die_init(dwarf, &die, errinfo)) return false;
        die.section = cu->unit.die.section;
        die.section_offset = parser->cur - parser->base;
        die.abbrev_code = getvar64(parser, NULL);
        die.abbrev_table = abtable;
        die.depth = depth;
        dwarf_abbrev_t *abbrev = dwarf_abbrev_table_find_abbrev_from_code(dwarf, abtable, die.abbrev_code);
        assert(abbrev); /* FIXME */
        struct Parser abparser;
        abparser.cur = dwarf->abbrev.section.base + abbrev->offset;
        abparser.size = -1;
        /* Skip over the abbreviation code */
        check(getvar64(&abparser, NULL) == die.abbrev_code);
        die.tag = getvar64(&abparser, NULL);
        die.has_children = get8(&abparser);
        if (dwarf->die_cb) {
            dwarf->die_cb(dwarf, &die);
        }
        if (die.has_children) depth++;
        while (peak16(&abparser)) {
            size_t i;
            dwarf_attr_t attr;
            attr.name = getvar64(&abparser, NULL);
            attr.form = getvar64(&abparser, NULL);
            switch (attr.form) {
            case DW_FORM_flag_present:
                attr.value.b = true;
                break;
            case DW_FORM_flag:
                attr.value.b = get8(parser);
                break;
            case DW_FORM_data1:
            case DW_FORM_ref1:
                attr.value.val = get8(parser);
                break;
            case DW_FORM_data2:
            case DW_FORM_ref2:
                attr.value.val = get16(parser);
                break;
            case DW_FORM_data4:
            case DW_FORM_ref4:
                attr.value.val = get32(parser);
                break;
            case DW_FORM_data8:
            case DW_FORM_ref8:
                attr.value.val = get64(parser);
                break;
            case DW_FORM_udata:
                attr.value.val = getvar64(parser, NULL);
                break;
            case DW_FORM_sdata: /* FIXME: Handel negative numbers */
                attr.value.val = getvar64(parser, NULL);
                break;
            case DW_FORM_addr:
                attr.value.addr = getaddr(parser);
                break;
            case DW_FORM_sec_offset:
                attr.value.off = get32(parser);
                break;
            case DW_FORM_string:
                attr.value.cstr = parser->cur;
                while (get8(parser));
                break;
            case DW_FORM_strp:
                attr.value.stroff = get32(parser);
                break;
            case DW_FORM_block1: /* TODO: Add value for these... */
                {
                    size_t len = get8(parser);
                    for (i=0; i < len; i++) get8(parser);
                }
                break;
            case DW_FORM_block2:
                {
                    size_t len = get16(parser);
                    for (i=0; i < len; i++) get8(parser);
                }
                break;
            case DW_FORM_block4:
                {
                    size_t len = get32(parser);
                    for (i=0; i < len; i++) get8(parser);
                }
                break;
            case DW_FORM_block:
                {
                    size_t len = getvar64(parser, NULL);
                    for (i=0; i < len; i++) get8(parser);
                }
                break;
            case DW_FORM_exprloc:
                { /* TODO */
                    uint64_t len = getvar64(parser, NULL);
                    while (len) {
                        get8(parser);
                        len--;
                    }
                }
                break;
            default: /* FIXME */
                fprintf(stderr, "sorry, unimplemented: %s\n", dwarf_get_symbol_name(DW_FORM, attr.form));
                abort();
            }
            if (dwarf->attr_cb && attr_cb_status != DW_CB_DONE) {
                attr_cb_status = dwarf->attr_cb(dwarf, &die, &attr);
            }
            if (die.attr_cb) {
                enum dw_cb_status status = die.attr_cb(dwarf, &die, &attr);
                if (status == DW_CB_DONE) die.attr_cb = NULL;
            }
        }
        get16(&abparser); /* Skip the null entry */
    } while (depth);

    return true;
}
static struct dwarf_abbreviation_table *dwarf_find_abbreviation_table_at_offset(struct dwarf *dwarf, dw_off_t aboff)
{
    /* FIXME: We should use some kind of hashmap here... */
    struct dwarf_abbreviation_table *abtable = NULL;
    size_t i;
    for (i=0; i < dwarf->abbrev.num_tables; i++) {
        struct dwarf_abbreviation_table *table = &dwarf->abbrev.tables[i];
        if (table->debug_abbrev_offset == aboff) abtable = table;
        if (table->debug_abbrev_offset >= aboff) break;
    }
    return abtable;
}
static bool dwarf_parse_info_section_cu(struct dwarf *dwarf, struct dwarf_section_info *info, dwarf_cu_t *cu, struct dwarf_errinfo *errinfo)
{
    struct Parser parser;
    parser.base = info->section.base + cu->unit.die.section_offset;
    parser.size = info->section.size - cu->unit.die.section_offset;
    parser.cur = parser.base;
    dw_u32_t length = get32(&parser);
    assert(cu->unit.die.length == length);
    cu->unit.version = get16(&parser);
    /*
    TODO: Actually check version
    assert(cu->unit.version == 2 || cu->unit.version == 4);

    NOTE: AFAIK, format is exactly the same, only form/value conventions are different
    The code as-is should be enough to just skip a section no matter the DWARF version
    */
    cu->unit.type = DWARF_UNITTYPE_COMPILE; /* Version 4 and below only have compilation units */
    cu->unit.debug_abbrev_offset = get32(&parser);
    cu->unit.address_size = get8(&parser);
    assert(cu->unit.address_size == 8); /* FIXME: Check */
    struct dwarf_abbreviation_table *abtable = dwarf_find_abbreviation_table_at_offset(dwarf, cu->unit.debug_abbrev_offset);
    if (!abtable) error(runtime_error("couldn't find an abbreviation table at offset %1 (for compilation unit at offset %2)", "II", cu->unit.debug_abbrev_offset, cu->unit.die.section_offset));
    return dwarf_parse_compilation_unit_from_abreviation_table(dwarf, cu, abtable, &parser, errinfo);
}
static bool dwarf_parse_info_section(struct dwarf *dwarf, struct dwarf_section_info *info, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!info->section.base)) error(argument_error(2, "info", __func__, "no segment data!"));

    struct Parser parser;
    parser.base = info->section.base;
    parser.size = info->section.size;
    parser.cur = parser.base;
    while (!done(&parser)) {
        dwarf_cu_t cu;
        if (!dwarf_cu_init(dwarf, &cu, errinfo)) return false;
        cu.unit.die.section = DWARF_SECTION_INFO;
        cu.unit.die.section_offset = parser.cur - parser.base;
        cu.unit.die.parent = NULL;
        cu.unit.die.depth = 0;
        cu.unit.die.length = get32(&parser);
        dwarf_parse_info_section_cu(dwarf, info, &cu, errinfo);
        parser.cur += cu.unit.die.length;
    }
    return true;
}
bool dwarf_parse_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

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
    }

    return true;
}
bool dwarf_parse(struct dwarf *dwarf, struct dwarf_errinfo *errinfo)
{
    if (has_error(errinfo)) return false;
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    dwarf_parse_section(dwarf, DWARF_SECTION_ARANGES, errinfo);
    dwarf_parse_section(dwarf, DWARF_SECTION_ABBREV, errinfo);
    dwarf_parse_section(dwarf, DWARF_SECTION_LINE, errinfo);
    dwarf_parse_section(dwarf, DWARF_SECTION_INFO, errinfo);
    dwarf_parse_section(dwarf, DWARF_SECTION_STR, errinfo);

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
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));

    switch (ns) {
    case DWARF_SECTION_ARANGES:
        dwarf->aranges.section = section;
        break;
    case DWARF_SECTION_INFO:
        dwarf->info.section = section;
        break;
    case DWARF_SECTION_LINE:
        dwarf->line.section = section;
        break;
    case DWARF_SECTION_ABBREV:
        dwarf->abbrev.section = section;
        break;
    case DWARF_SECTION_STR:
        dwarf->str.section = section;
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
    if (dw_unlikely(!dwarf)) error(argument_error(1, "dwarf", __func__, "pointer is NULL"));
    if (dw_unlikely(!allocator)) error(argument_error(2, "allocator", __func__, "must provide an allocator"));

    *dwarf = NULL;
    int status = (*allocator)(allocator, &allocation_request, (void **)dwarf);
    if (dw_unlikely(status < 0 || !dwarf)) error(allocator_error(allocator, sizeof(struct dwarf), *dwarf, "failed to allocate dwarf state"));
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
    if (dw_unlikely(!dwarf)) return; /* You're a weirdo */
    if (dw_unlikely(!*dwarf)) return; /* We already freed it */

    dw_alloc_t *allocator = (*dwarf)->allocator;
    if (allocator) {
        (void)(*allocator)(allocator, &deallocation_request, (void **)dwarf);
        /* ^ Ignore status for deallocation call */
        /* Give allocator a chance to free internal state */
        (*allocator)(allocator, NULL, NULL);
    }
    *dwarf = NULL; /* Ensure it is `NULL` */
}
