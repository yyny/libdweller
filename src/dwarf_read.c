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

#define dwarf_header_length_size(dwarf64) (dwarf64 == 64 ? 4 + 8 : 4)

DWSTATIC(bool) dwarf_read_attr(struct dwarf *dwarf, dw_stream_t *stream, dwarf_attr_t *attr)
{
    size_t i;
    switch (attr->form) {
    case DW_FORM_flag_present:
        attr->value.b = true;
        break;
    case DW_FORM_flag:
        attr->value.b = dw_stream_get8(stream);
        break;
    case DW_FORM_data1:
    case DW_FORM_ref1:
        attr->value.val = dw_stream_get8(stream);
        break;
    case DW_FORM_data2:
    case DW_FORM_ref2:
        attr->value.val = dw_stream_get16(stream);
        break;
    case DW_FORM_data4:
    case DW_FORM_ref4:
        attr->value.val = dw_stream_get32(stream);
        break;
    case DW_FORM_data8:
    case DW_FORM_ref8:
        attr->value.val = dw_stream_get64(stream);
        break;
    case DW_FORM_udata:
        attr->value.val = dw_stream_getleb128_unsigned(stream, NULL);
        break;
    case DW_FORM_sdata:
        attr->value.val = dw_stream_getleb128_signed(stream, NULL);
        break;
    case DW_FORM_addr:
        attr->value.addr = (void *)dw_stream_getaddr(stream, 64); /* TODO: Test if we are in 32 or 64 bit binary */
        break;
    case DW_FORM_sec_offset:
        attr->value.off = dw_stream_get32(stream); /* TODO: Test if dwarf64 */
        break;
    case DW_FORM_string:
        attr->value.str.section = stream->section;
        attr->value.str.off = dw_stream_tell(stream);
        while (dw_stream_get8(stream));
        attr->value.str.len = dw_stream_tell(stream) - attr->value.str.off - 1;
        break;
    case DW_FORM_strp:
        attr->value.stroff = dw_stream_get32(stream);
        break;
    case DW_FORM_block1: /* TODO: Add struct member for these... We just discarding right now */
        {
            size_t len = dw_stream_get8(stream);
            for (i=0; i < len; i++) dw_stream_get8(stream);
        }
        break;
    case DW_FORM_block2:
        {
            size_t len = dw_stream_get16(stream);
            for (i=0; i < len; i++) dw_stream_get8(stream);
        }
        break;
    case DW_FORM_block4:
        {
            size_t len = dw_stream_get32(stream);
            for (i=0; i < len; i++) dw_stream_get8(stream);
        }
        break;
    case DW_FORM_block:
        {
            size_t len = dw_stream_getleb128_unsigned(stream, NULL);
            for (i=0; i < len; i++) dw_stream_get8(stream);
        }
        break;
    case DW_FORM_exprloc:
        { /* TODO: Evaluate this and store it somehow */
            uint64_t len = dw_stream_getleb128_unsigned(stream, NULL);
            while (len) {
                dw_stream_get8(stream);
                len--;
            }
        }
        break;
    default: /* FIXME */
        fprintf(stderr, "sorry, unimplemented: %s\n", dwarf_get_symbol_name(DW_FORM, attr->form));
        abort();
    }

    return true;
}
DWSTATIC(bool) dwarf_read_line_program_header(struct dwarf *dwarf, dw_stream_t *stream, struct dwarf_line_program *line_program, struct dwarf_errinfo *errinfo)
{
    line_program->section_offset = dw_stream_tell(stream);
    line_program->length = dw_stream_get32(stream);
    line_program->version = dw_stream_get16(stream);
    switch (line_program->version) {
    case 2:
    case 3:
    case 4: /* TODO: Test version 4 better */
    case 5: /* TODO: Test version 5 better */
        break;
    default:
        error(runtime_error("unsupported line program version: %1", "I", line_program->version));
    }
    if (line_program->version >= 5) {
        line_program->address_size = dw_stream_get8(stream);
        line_program->segment_selector_size = dw_stream_get8(stream);
    }
    line_program->header_length = dw_stream_get32(stream); /* TODO: 64 bits in dwarf64 */
    line_program->instruction_size = dw_stream_get8(stream);
    if (line_program->version >= 4) {
        line_program->maximum_operations_per_instruction = dw_stream_get8(stream);
    }
    line_program->default_is_stmt = dw_stream_get8(stream);
    line_program->line_base = dw_stream_get8(stream);
    line_program->line_range = dw_stream_get8(stream);
    line_program->first_special_opcode = dw_stream_get8(stream);
    uint8_t num_basic_opcodes = line_program->first_special_opcode - 1;
    line_program->basic_opcode_argcount = dw_malloc(dwarf, num_basic_opcodes * sizeof(uint8_t));
    size_t i;
    for (i=0; i < num_basic_opcodes; i++) {
        line_program->basic_opcode_argcount[i] = dw_stream_get8(stream);
    }
    if (line_program->version >= 5) {
        /* FIXME: Much of this code is duplicated for version < 5 */
        dwarf_attr_t attr;
        line_program->directorydata_format_count = dw_stream_get8(stream);
        line_program->directorydata_format = dw_malloc(dwarf, line_program->directorydata_format_count * sizeof(struct dwarf_line_program_format));
        for (i=0; i < line_program->directorydata_format_count; i++) {
            line_program->directorydata_format[i].name = dw_stream_getleb128_unsigned(stream, NULL);
            line_program->directorydata_format[i].form = dw_stream_getleb128_unsigned(stream, NULL);
        }
        for (i=0; i < line_program->directorydata_format_count; i++) {
            attr.name = line_program->directorydata_format[i].name;
            attr.form = line_program->directorydata_format[i].form;
            dwarf_read_attr(dwarf, stream, &attr);
            switch (attr.name) {
            case DW_LNCT_path:
                switch (attr.form) {
                case DW_FORM_string:
                case DW_FORM_strp:
                case DW_FORM_line_strp:
                    line_program->include_directories[i].form = attr.form;
                    line_program->include_directories[i].value = attr.value;
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
        line_program->num_include_directories = dw_stream_getleb128_unsigned(stream, NULL);
        line_program->include_directories = dw_malloc(dwarf, line_program->num_include_directories * sizeof(dw_str_t));
        for (i=0; i < line_program->num_include_directories; i++) {
            attr.name = line_program->directorydata_format[i].name;
            attr.form = line_program->directorydata_format[i].form;
            dwarf_read_attr(dwarf, stream, &attr);
        }
        line_program->filedata_format_count = dw_stream_get8(stream);
        line_program->filedata_format = dw_malloc(dwarf, line_program->filedata_format_count * sizeof(struct dwarf_line_program_format));
        for (i=0; i < line_program->filedata_format_count; i++) {
            line_program->directorydata_format[i].name = dw_stream_getleb128_unsigned(stream, NULL);
            line_program->directorydata_format[i].form = dw_stream_getleb128_unsigned(stream, NULL);
        }
        line_program->num_files = dw_stream_getleb128_unsigned(stream, NULL);
        line_program->files = dw_malloc(dwarf, line_program->num_files * sizeof(struct dwarf_fileinfo));
        for (i=0; i < line_program->num_files; i++) {
            attr.name = line_program->directorydata_format[i].name;
            attr.form = line_program->directorydata_format[i].form;
            dwarf_read_attr(dwarf, stream, &attr);
            switch (attr.name) {
            case DW_LNCT_path:
                switch (attr.form) {
                case DW_FORM_string:
                    line_program->files[i].name.section = DWARF_SECTION_LINE;
                    line_program->files[i].name.off = attr.value.str.off;
                    abort(); /* FIXME: Not fully implemented, but really easy */
                    break;
                case DW_FORM_strp:
                line_program->files[i].name.section = DWARF_SECTION_STR;
                    line_program->files[i].name.off = attr.value.stroff;
                    line_program->files[i].name.len = -1;
                    break;
                case DW_FORM_line_strp:
                    line_program->files[i].name.section = DWARF_SECTION_LINESTR;
                    line_program->files[i].name.off = attr.value.stroff;
                    line_program->files[i].name.len = -1;
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
                    line_program->files[i].include_directory_idx = attr.value.val;
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
                    line_program->files[i].last_modification_time = attr.value.val;
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
                    line_program->files[i].file_size = attr.value.val;
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
        line_program->directorydata_format_count = 0;
        line_program->directorydata_format = NULL;
        line_program->filedata_format_count = 0;
        line_program->filedata_format = NULL;
        line_program->num_include_directories = 0;
        line_program->include_directories = NULL;
        line_program->files = NULL;
        line_program->num_files = 0;
        line_program->directorydata = NULL;
        line_program->filedata = NULL;
        while (dw_stream_peak8(stream)) {
            dw_i64_t start = dw_stream_tell(stream);
            struct dwarf_pathinfo path;
            path.form = DW_FORM_string,
            path.value.str.section = DWARF_SECTION_LINE;
            path.value.str.off = start;
            while (dw_stream_get8(stream));
            path.value.str.len = dw_stream_tell(stream) - start - 1;
            line_program->total_include_path_size += path.value.str.len;
            /* TODO: Allocate as OBSTACK */
            line_program->include_directories = dw_realloc(dwarf, line_program->include_directories, (line_program->num_include_directories + 1) * sizeof(struct dwarf_pathinfo));
            line_program->include_directories[line_program->num_include_directories++] = path;
        }
        dw_stream_get8(stream);
        while (dw_stream_peak8(stream)) {
            struct dwarf_fileinfo info;
            dw_i64_t start = dw_stream_tell(stream);
            info.name.section = DWARF_SECTION_LINE;
            info.name.off = start;
            while (dw_stream_get8(stream));
            info.name.len = dw_stream_tell(stream) - start - 1; /* FIXME: This will be overwritten on the next call to dw_stream!!! */
            line_program->total_file_path_size += info.name.len;
            info.include_directory_idx = dw_stream_getleb128_unsigned(stream, NULL);
            info.last_modification_time = dw_stream_getleb128_unsigned(stream, NULL);
            info.file_size = dw_stream_getleb128_unsigned(stream, NULL);
            /* TODO: Allocate as OBSTACK */
            line_program->files = dw_realloc(dwarf, line_program->files, (line_program->num_files + 1) * sizeof(struct dwarf_fileinfo));
            line_program->files[line_program->num_files++] = info;
        }
        dw_stream_get8(stream); /* Skip NUL */
    }
    line_program->header_size = dw_stream_tell(stream) - line_program->section_offset;
    return true;
}
DWSTATIC(void) dwarf_line_program_init(struct dwarf *dwarf, struct dwarf_line_program *line_program)
{
    memset(line_program, 0x00, sizeof(*line_program));
}
DWSTATIC(void) dwarf_line_program_state_init(struct dwarf *dwarf, struct dwarf_line_program *program, struct dwarf_line_program_state *state)
{
    memset(state, 0x00, sizeof(*state));
    state->file = 1;
    state->line = 1;
    state->is_stmt = program->default_is_stmt;
}
DWSTATIC(bool) dwarf_read_line_program_row(struct dwarf *dwarf, dw_stream_t *stream, struct dwarf_line_program *line_program, struct dwarf_line_program_state *state, struct dwarf_line_program_state *state_out, struct dwarf_errinfo *errinfo)
{
    bool have_row = false;
    while (!have_row && dw_stream_tell(stream) < line_program->section_offset + line_program->length + dwarf_header_length_size(line_program->dwarf64)) {
        int basic_opcode = dw_stream_get8(stream);
        if (basic_opcode >= line_program->first_special_opcode) { /* This is a special opcode, it takes no arguments */
            int special_opcode = basic_opcode - line_program->first_special_opcode;
            int line_increment = line_program->line_base + (special_opcode % line_program->line_range);
            int address_increment = (special_opcode / line_program->line_range) * line_program->instruction_size;
            state->line += line_increment;
            state->address += address_increment;
            dw_i64_t off = dw_stream_tell(stream);
            *state_out = *state;
            have_row = true;
            dw_stream_seek(stream, off);
            state->basic_block = false;
            state->prologue_end = false;
            state->epilogue_begin = false;
            state->discriminator = false;
        } else if (basic_opcode == DW_LNS_fixed_advance_pc) { /* See docs */
            int address_increment = dw_stream_get16(stream);
            state->address += address_increment;
        } else if (basic_opcode) { /* This is a basic opcode */
            uint8_t i;
            uint64_t nargs = line_program->basic_opcode_argcount[basic_opcode - 1]; /* Array is 1-indexed */
            uint64_t *args = dw_malloc(dwarf, nargs * sizeof(uint64_t));
            dw_i64_t argoff = dw_stream_tell(stream);
            for (i=0; i < nargs; i++) {
                /* Getting the offsets to arguments other than the first one is
                 * trivial with the second argument to dw_stream_getleb128_unsigned.
                 * We only need to reparse the first one
                 */
                args[i] = dw_stream_getleb128_unsigned(stream, NULL);
            }
            switch (basic_opcode) {
            /* Same as basic opcode with `line_increment` and
             * `address_increment` as zero
             */
            case DW_LNS_copy:
                *state_out = *state;
                have_row = true;
                state->basic_block = false;
                break;
            case DW_LNS_advance_pc:
                {
                    assert(nargs == 1); /* TODO: Do this test when the basic_opcode_argcount gets filled? */
                    int address_increment = args[0] * line_program->instruction_size;
                    state->address += address_increment;
                }
                break;
            case DW_LNS_advance_line:
                {
                    dw_i64_t off = dw_stream_tell(stream);
                    dw_stream_seek(stream, argoff);
                    state->line += dw_stream_getleb128_signed(stream, NULL);
                    dw_stream_seek(stream, off);
                }
                break;
            case DW_LNS_set_file:
                state->file = args[0];
                break;
            case DW_LNS_set_column:
                state->column = args[0];
                break;
            case DW_LNS_negate_stmt:
                state->is_stmt = !state->is_stmt;
                break;
            case DW_LNS_set_basic_block:
                state->basic_block = true;
                break;
            case DW_LNS_const_add_pc:
                {
                    int address_increment = ((255 - line_program->first_special_opcode) / line_program->line_range) * line_program->instruction_size;
                    state->address += address_increment;
                }
                break;
            case DW_LNS_set_prologue_end:
                state->prologue_end = true;
                break;
            case DW_LNS_set_epilogue_begin:
                state->epilogue_begin = true;
                break;
            case DW_LNS_set_isa:
                state->isa = args[0];
                break;
            case DW_LNS_fixed_advance_pc: /* This is already handled and just to shut up compiler warnings */
            default:
                break;
            }
            dw_free(dwarf, args);
        } else { /* This is an extended opcode */
            uint64_t i;
            uint64_t extended_opcode_length = dw_stream_getleb128_unsigned(stream, NULL);
            uint8_t extended_opcode = dw_stream_get8(stream);
            assert(extended_opcode_length != 0);
            uint8_t *args = dw_malloc(dwarf, extended_opcode_length - 1);
            for (i=0; i < extended_opcode_length - 1; i++) {
                args[i] = dw_stream_get8(stream);
            }
            switch (extended_opcode) {
            case DW_LNE_end_sequence:
                state->end_sequence = true;
                *state_out = *state;
                have_row = true;
                dwarf_line_program_state_init(dwarf, line_program, state);
                break;
            case DW_LNE_set_address: /* FIXME: Check that args is actually an address */
                state->address = *(uint64_t *)args;
                break;
            case DW_LNE_define_file:
                /* TODO */
                break;
            case DW_LNE_set_discriminator:
                {
                    state->discriminator = dw_leb128_parse_unsigned(args, extended_opcode_length - 1, NULL);
                }
                break;
            default:
                break;
            }
        }
    }
    if (!have_row) {
        *state_out = *state;
    }
    return true;
}
