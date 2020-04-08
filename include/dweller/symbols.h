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
#ifndef DWELLER_SYMBOLS_H
#define DWELLER_SYMBOLS_H
#include <dweller/macros.h>

enum dwarf_symbol_namespace {
#define DW_UNKNOWN 0
    /* Introduced in DWARF1 */
    DW_AT = 1,
    DW_LANG,
    DW_TAG,
    DW_ORD,
    /* Only in DWARF1 */
#if 0
    DW_FT,
    DW_FMT,
    DW_MOD,
#endif
    /* Introduced in DWARF2 */
    DW_ACCESS,
    DW_ATE,
    DW_CC,
    DW_CFA,
    DW_CHILDREN,
    DW_DSC,
    DW_FORM,
    DW_ID,
    DW_INL,
    DW_LNE, /* Line number extended opcode name */
    DW_LNS, /* Line number standard opcode name. */
    DW_MACINFO,
    DW_OP,
    DW_VIRTUALITY,
    DW_VIS,
    /* Introduced in DWARF3 */
    DW_DS,
    DW_END,
    /* Introduced in DWARF5 */
    DW_UT,

    /* GNU Exception Handling */
    DW_EH  = 0x80,
};
#define DW_AT_SYMBOLS(SYMBOL) \
    SYMBOL(sibling,                  0x01) \
    SYMBOL(location,                 0x02) \
    SYMBOL(name,                     0x03) \
    SYMBOL(ordering,                 0x09) \
    SYMBOL(subscr_data,              0x0a) \
    SYMBOL(byte_size,                0x0b) \
    SYMBOL(bit_offset,               0x0c) \
    SYMBOL(bit_size,                 0x0d) \
    SYMBOL(element_list,             0x0f) \
    SYMBOL(stmt_list,                0x10) \
    SYMBOL(low_pc,                   0x11) \
    SYMBOL(high_pc,                  0x12) \
    SYMBOL(language,                 0x13) \
    SYMBOL(member,                   0x14) \
    SYMBOL(discr,                    0x15) \
    SYMBOL(discr_value,              0x16) \
    SYMBOL(visibility,               0x17) \
    SYMBOL(import,                   0x18) \
    SYMBOL(string_length,            0x19) \
    SYMBOL(common_reference,         0x1a) \
    SYMBOL(comp_dir,                 0x1b) \
    SYMBOL(const_value,              0x1c) \
    SYMBOL(containing_type,          0x1d) \
    SYMBOL(default_value,            0x1e) \
    SYMBOL(inline,                   0x20) \
    SYMBOL(is_optional,              0x21) \
    SYMBOL(lower_bound,              0x22) \
    SYMBOL(producer,                 0x25) \
    SYMBOL(prototyped,               0x27) \
    SYMBOL(return_addr,              0x2a) \
    SYMBOL(start_scope,              0x2c) \
    SYMBOL(bit_stride,               0x2e) /* DWARF3 name */ \
    SYMBOL(upper_bound,              0x2f) \
    SYMBOL(abstract_origin,          0x31) \
    SYMBOL(accessibility,            0x32) \
    SYMBOL(address_class,            0x33) \
    SYMBOL(artificial,               0x34) \
    SYMBOL(base_types,               0x35) \
    SYMBOL(calling_convention,       0x36) \
    SYMBOL(count,                    0x37) \
    SYMBOL(data_member_location,     0x38) \
    SYMBOL(decl_column,              0x39) \
    SYMBOL(decl_file,                0x3a) \
    SYMBOL(decl_line,                0x3b) \
    SYMBOL(declaration,              0x3c) \
    SYMBOL(discr_list,               0x3d) /* DWARF2 */ \
    SYMBOL(encoding,                 0x3e) \
    SYMBOL(external,                 0x3f) \
    SYMBOL(frame_base,               0x40) \
    SYMBOL(friend,                   0x41) \
    SYMBOL(identifier_case,          0x42) \
    SYMBOL(macro_info,               0x43) /* DWARF{234} (not DWARF5) */ \
    SYMBOL(namelist_item,            0x44) \
    SYMBOL(priority,                 0x45) \
    SYMBOL(segment,                  0x46) \
    SYMBOL(specification,            0x47) \
    SYMBOL(static_link,              0x48) \
    SYMBOL(type,                     0x49) \
    SYMBOL(use_location,             0x4a) \
    SYMBOL(variable_parameter,       0x4b) \
    SYMBOL(virtuality,               0x4c) \
    SYMBOL(vtable_elem_location,     0x4d) \
    SYMBOL(allocated,                0x4e) /* DWARF3 */ \
    SYMBOL(associated,               0x4f) /* DWARF3 */ \
    SYMBOL(data_location,            0x50) /* DWARF3 */ \
    SYMBOL(byte_stride,              0x51) /* DWARF3f */ \
    SYMBOL(entry_pc,                 0x52) /* DWARF3 */ \
    SYMBOL(use_UTF8,                 0x53) /* DWARF3 */ \
    SYMBOL(extension,                0x54) /* DWARF3 */ \
    SYMBOL(ranges,                   0x55) /* DWARF3 */ \
    SYMBOL(trampoline,               0x56) /* DWARF3 */ \
    SYMBOL(call_column,              0x57) /* DWARF3 */ \
    SYMBOL(call_file,                0x58) /* DWARF3 */ \
    SYMBOL(call_line,                0x59) /* DWARF3 */ \
    SYMBOL(description,              0x5a) /* DWARF3 */ \
    SYMBOL(binary_scale,             0x5b) /* DWARF3f */ \
    SYMBOL(decimal_scale,            0x5c) /* DWARF3f */ \
    SYMBOL(small,                    0x5d) /* DWARF3f */ \
    SYMBOL(decimal_sign,             0x5e) /* DWARF3f */ \
    SYMBOL(digit_count,              0x5f) /* DWARF3f */ \
    SYMBOL(picture_string,           0x60) /* DWARF3f */ \
    SYMBOL(mutable,                  0x61) /* DWARF3f */ \
    SYMBOL(threads_scaled,           0x62) /* DWARF3f */ \
    SYMBOL(explicit,                 0x63) /* DWARF3f */ \
    SYMBOL(object_pointer,           0x64) /* DWARF3f */ \
    SYMBOL(endianity,                0x65) /* DWARF3f */ \
    SYMBOL(elemental,                0x66) /* DWARF3f */ \
    SYMBOL(pure,                     0x67) /* DWARF3f */ \
    SYMBOL(recursive,                0x68) /* DWARF3f */ \
    SYMBOL(signature,                0x69) /* DWARF4 */ \
    SYMBOL(main_subprogram,          0x6a) /* DWARF4 */ \
    SYMBOL(data_bit_offset,          0x6b) /* DWARF4 */ \
    SYMBOL(const_expr,               0x6c) /* DWARF4 */ \
    SYMBOL(enum_class,               0x6d) /* DWARF4 */ \
    SYMBOL(linkage_name,             0x6e) /* DWARF4 */ \
    SYMBOL(string_length_bit_size,   0x6f) /* DWARF5 */ \
    SYMBOL(string_length_byte_size,  0x70) /* DWARF5 */ \
    SYMBOL(rank,                     0x71) /* DWARF5 */ \
    SYMBOL(str_offsets_base,         0x72) /* DWARF5 */ \
    SYMBOL(addr_base,                0x73) /* DWARF5 */ \
    SYMBOL(rnglists_base,            0x74) /* DWARF5 */ \
    SYMBOL(dwo_id,                   0x75) /* DWARF4!*/ \
    SYMBOL(dwo_name,                 0x76) /* DWARF5 */ \
    SYMBOL(reference,                0x77) /* DWARF5 */ \
    SYMBOL(rvalue_reference,         0x78) /* DWARF5 */ \
    SYMBOL(macros,                   0x79) /* DWARF5 */ \
    SYMBOL(call_all_calls,           0x7a) /* DWARF5 */ \
    SYMBOL(call_all_source_calls,    0x7b) /* DWARF5 */ \
    SYMBOL(call_all_tail_calls,      0x7c) /* DWARF5 */ \
    SYMBOL(call_return_pc,           0x7d) /* DWARF5 */ \
    SYMBOL(call_value,               0x7e) /* DWARF5 */ \
    SYMBOL(call_origin,              0x7f) /* DWARF5 */ \
    SYMBOL(call_parameter,           0x80) /* DWARF5 */ \
    SYMBOL(call_pc,                  0x81) /* DWARF5 */ \
    SYMBOL(call_tail_call,           0x82) /* DWARF5 */ \
    SYMBOL(call_target,              0x83) /* DWARF5 */ \
    SYMBOL(call_target_clobbered,    0x84) /* DWARF5 */ \
    SYMBOL(call_data_location,       0x85) /* DWARF5 */ \
    SYMBOL(call_data_value,          0x86) /* DWARF5 */ \
    SYMBOL(noreturn,                 0x87) /* DWARF5 */ \
    SYMBOL(alignment,                0x88) /* DWARF5 */ \
    SYMBOL(export_symbols,           0x89) /* DWARF5 */ \
    SYMBOL(deleted,                  0x8a) /* DWARF5 */ \
    SYMBOL(defaulted,                0x8b) /* DWARF5 */ \
    SYMBOL(loclists_base,            0x8c) /* DWARF5 */
#define DW_TAG_SYMBOLS(SYMBOL) \
    SYMBOL(array_type,                0x01) \
    SYMBOL(class_type,                0x02) \
    SYMBOL(entry_point,               0x03) \
    SYMBOL(enumeration_type,          0x04) \
    SYMBOL(formal_parameter,          0x05) \
    SYMBOL(imported_declaration,      0x08) \
    SYMBOL(label,                     0x0a) \
    SYMBOL(lexical_block,             0x0b) \
    SYMBOL(member,                    0x0d) \
    SYMBOL(pointer_type,              0x0f) \
    SYMBOL(reference_type,            0x10) \
    SYMBOL(compile_unit,              0x11) \
    SYMBOL(string_type,               0x12) \
    SYMBOL(structure_type,            0x13) \
    SYMBOL(subroutine_type,           0x15) \
    SYMBOL(typedef,                   0x16) \
    SYMBOL(union_type,                0x17) \
    SYMBOL(unspecified_parameters,    0x18) \
    SYMBOL(variant,                   0x19) \
    SYMBOL(common_block,              0x1a) \
    SYMBOL(common_inclusion,          0x1b) \
    SYMBOL(inheritance,               0x1c) \
    SYMBOL(inlined_subroutine,        0x1d) \
    SYMBOL(module,                    0x1e) \
    SYMBOL(ptr_to_member_type,        0x1f) \
    SYMBOL(set_type,                  0x20) \
    SYMBOL(subrange_type,             0x21) \
    SYMBOL(with_stmt,                 0x22) \
    SYMBOL(access_declaration,        0x23) \
    SYMBOL(base_type,                 0x24) \
    SYMBOL(catch_block,               0x25) \
    SYMBOL(const_type,                0x26) \
    SYMBOL(constant,                  0x27) \
    SYMBOL(enumerator,                0x28) \
    SYMBOL(file_type,                 0x29) \
    SYMBOL(friend,                    0x2a) \
    SYMBOL(namelist,                  0x2b) \
    SYMBOL(namelist_item,             0x2c) /* DWARF{23} spelling */ \
    SYMBOL(packed_type,               0x2d) \
    SYMBOL(subprogram,                0x2e) \
    SYMBOL(template_type_parameter,   0x2f) /* DWARF{23} spelling*/ \
    SYMBOL(template_value_parameter,  0x30) /* DWARF{23} spelling*/ \
    SYMBOL(thrown_type,               0x31) \
    SYMBOL(try_block,                 0x32) \
    SYMBOL(variant_part,              0x33) \
    SYMBOL(variable,                  0x34) \
    SYMBOL(volatile_type,             0x35) \
    SYMBOL(dwarf_procedure,           0x36) /* DWARF3 */ \
    SYMBOL(restrict_type,             0x37) /* DWARF3 */ \
    SYMBOL(interface_type,            0x38) /* DWARF3 */ \
    SYMBOL(namespace,                 0x39) /* DWARF3 */ \
    SYMBOL(imported_module,           0x3a) /* DWARF3 */ \
    SYMBOL(unspecified_type,          0x3b) /* DWARF3 */ \
    SYMBOL(partial_unit,              0x3c) /* DWARF3 */ \
    SYMBOL(imported_unit,             0x3d) /* DWARF3 */ \
    SYMBOL(condition,                 0x3f) /* DWARF3f */ \
    SYMBOL(shared_type,               0x40) /* DWARF3f */ \
    SYMBOL(type_unit,                 0x41) /* DWARF4 */ \
    SYMBOL(rvalue_reference_type,     0x42) /* DWARF4 */ \
    SYMBOL(template_alias,            0x43) /* DWARF4 */ \
    SYMBOL(coarray_type,              0x44) /* DWARF5 */ \
    SYMBOL(generic_subrange,          0x45) /* DWARF5 */ \
    SYMBOL(dynamic_type,              0x46) /* DWARF5 */ \
    SYMBOL(atomic_type,               0x47) /* DWARF5 */ \
    SYMBOL(call_site,                 0x48) /* DWARF5 */ \
    SYMBOL(call_site_parameter,       0x49) /* DWARF5 */ \
    SYMBOL(skeleton_unit,             0x4a) /* DWARF5 */ \
    SYMBOL(immutable_type,            0x4b) /* DWARF5 */

#define DW_CHILDREN_SYMBOLS(SYMBOL) \
    SYMBOL(no,   0x00)              \
    SYMBOL(yes,  0x01)
#define DW_FORM_SYMBOLS(SYMBOL) \
    SYMBOL(addr,            0x01) \
    SYMBOL(block2,          0x03) \
    SYMBOL(block4,          0x04) \
    SYMBOL(data2,           0x05) \
    SYMBOL(data4,           0x06) \
    SYMBOL(data8,           0x07) \
    SYMBOL(string,          0x08) \
    SYMBOL(block,           0x09) \
    SYMBOL(block1,          0x0a) \
    SYMBOL(data1,           0x0b) \
    SYMBOL(flag,            0x0c) \
    SYMBOL(sdata,           0x0d) \
    SYMBOL(strp,            0x0e) \
    SYMBOL(udata,           0x0f) \
    SYMBOL(ref_addr,        0x10) \
    SYMBOL(ref1,            0x11) \
    SYMBOL(ref2,            0x12) \
    SYMBOL(ref4,            0x13) \
    SYMBOL(ref8,            0x14) \
    SYMBOL(ref_udata,       0x15) \
    SYMBOL(indirect,        0x16) \
    SYMBOL(sec_offset,      0x17) /* DWARF4 */ \
    SYMBOL(exprloc,         0x18) /* DWARF4 */ \
    SYMBOL(flag_present,    0x19) /* DWARF4 */ \
    SYMBOL(strx,            0x1a) /* DWARF5 */ \
    SYMBOL(addrx,           0x1b) /* DWARF5 */ \
    SYMBOL(ref_sup4,        0x1c) /* DWARF5 */ \
    SYMBOL(strp_sup,        0x1d) /* DWARF5 */ \
    SYMBOL(data16,          0x1e) /* DWARF5 */ \
    SYMBOL(line_strp,       0x1f) /* DWARF5 */ \
    SYMBOL(ref_sig8,        0x20) /* DWARF4 */ \
    SYMBOL(implicit_const,  0x21) /* DWARF5 */ \
    SYMBOL(loclistx,        0x22) /* DWARF5 */ \
    SYMBOL(rnglistx,        0x23) /* DWARF5 */ \
    SYMBOL(ref_sup8,        0x24) /* DWARF5 */ \
    SYMBOL(strx1,           0x25) /* DWARF5 */ \
    SYMBOL(strx2,           0x26) /* DWARF5 */ \
    SYMBOL(strx3,           0x27) /* DWARF5 */ \
    SYMBOL(strx4,           0x28) /* DWARF5 */ \
    SYMBOL(addrx1,          0x29) /* DWARF5 */ \
    SYMBOL(addrx2,          0x2a) /* DWARF5 */ \
    SYMBOL(addrx3,          0x2b) /* DWARF5 */ \
    SYMBOL(addrx4,          0x2c) /* DWARF5 */
#define DW_LNE_SYMBOLS(SYMBOL) \
    SYMBOL(end_sequence,       0x01) \
    SYMBOL(set_address,        0x02) \
    SYMBOL(define_file,        0x03) /* DWARF4 and earlier only */ \
    SYMBOL(set_discriminator,  0x04) /* DWARF4 */
#define DW_LNS_SYMBOLS(SYMBOL) \
    SYMBOL(copy,                0x01) \
    SYMBOL(advance_pc,          0x02) \
    SYMBOL(advance_line,        0x03) \
    SYMBOL(set_file,            0x04) \
    SYMBOL(set_column,          0x05) \
    SYMBOL(negate_stmt,         0x06) \
    SYMBOL(set_basic_block,     0x07) \
    SYMBOL(const_add_pc,        0x08) \
    SYMBOL(fixed_advance_pc,    0x09) \
    SYMBOL(set_prologue_end,    0x0a) /* DWARF3 */ \
    SYMBOL(set_epilogue_begin,  0x0b) /* DWARF3 */ \
    SYMBOL(set_isa,             0x0c) /* DWARF3 */
#define DW_VIS_SYMBOLS(SYMBOL) \
    SYMBOL(local,      0x00)   \
    SYMBOL(exported,   0x01)   \
    SYMBOL(qualified,  0x02)

#define DW_UT_SYMBOLS(SYMBOL) \
    SYMBOL(compile,        0x01) \
    SYMBOL(type,           0x02) \
    SYMBOL(partial,        0x03) \
    SYMBOL(skeleton,       0x04) \
    SYMBOL(split_compile,  0x05) \
    SYMBOL(split_type,     0x06)

/* DWARF1 only */
#if 0
#define DW_AT_RESERVED_04   0x04
#define DW_AT_fund_type     0x05
#define DW_AT_mod_fund_type 0x06
#define DW_AT_user_def_type 0x07
#define DW_AT_mod_u_d_type  0x08
#define DW_AT_RESERVED_0D   0x0d
#define DW_AT_RESERVED_1F   0x1f
#define DW_AT_RESERVED_23   0x23
#define DW_AT_RESERVED_24   0x24
#define DW_AT_RESERVED_26   0x26
#define DW_AT_RESERVED_28   0x28
#define DW_AT_RESERVED_29   0x29
#define DW_AT_RESERVED_2B   0x2b
#define DW_AT_RESERVED_2D   0x2d
#define DW_AT_virtual       0x30
/* Use DW_AT_rnglists_base, DW_AT_ranges_base is obsolete as
 * it was only used in some DWARF5 drafts, not the final DWARF5.
 */
#define DW_AT_ranges_base   0x74

#define DW_TAG_global_subroutine  0x06
#define DW_TAG_global_variable    0x07
#define DW_TAG_RESERVED_09        0x09
#define DW_TAG_local_variable     0x0c
#define DW_TAG_RESERVED_0E        0x0e
#define DW_TAG_subroutine         0x14

#define DW_FORM_ref 0x02
#endif

#define DW_AT_stride_size 0x2e /* DWARF2 name */
#define DW_AT_stride      0x51 /* DWARF3 (do not use) */
/*  Early releases of libdwarf had the
 *  following misspelled with a trailing 's'
 */
#define DW_TAG_namelist_items 0x2c /* SGI misspelling/typo */
/*  The DWARF2 document had two spellings of the following
 *  two TAGs, DWARF3 specifies the longer spelling.
 */
#define DW_TAG_template_type_param 0x2f /* DWARF2 spelling*/
#define DW_TAG_template_value_param 0x30 /* DWARF2 spelling*/
/*  Do not use DW_TAG_mutable_type
 */
#define DW_TAG_mutable_type 0x3e /* Withdrawn from DWARF3 by DWARF3f. */

#define DW_FORM_GNU_addr_index 0x1f01 /* GNU extension in debug_info.dwo.*/
#define DW_FORM_GNU_str_index  0x1f02 /* GNU extension, somewhat like DW_FORM_strp */
#define DW_FORM_GNU_ref_alt    0x1f20 /* GNU extension. Offset in .debug_info. */
#define DW_FORM_GNU_strp_alt   0x1f21 /* GNU extension. Offset in .debug_str of another object file. */

#define DW_LNE_lo_user                  0x80 /* DWARF3 */
#define DW_LNE_hi_user                  0xff /* DWARF3 */

/* HP extensions. */
#define DW_LNE_HP_negate_is_UV_update      0x11 /* 17 HP */
#define DW_LNE_HP_push_context             0x12 /* 18 HP */
#define DW_LNE_HP_pop_context              0x13 /* 19 HP */
#define DW_LNE_HP_set_file_line_column     0x14 /* 20 HP */
#define DW_LNE_HP_set_routine_name         0x15 /* 21 HP */
#define DW_LNE_HP_set_sequence             0x16 /* 22 HP */
#define DW_LNE_HP_negate_post_semantics    0x17 /* 23 HP */
#define DW_LNE_HP_negate_function_exit     0x18 /* 24 HP */
#define DW_LNE_HP_negate_front_end_logical 0x19 /* 25 HP */
#define DW_LNE_HP_define_proc              0x20 /* 32 HP */

#define DW_LNE_HP_source_file_correlation  0x80 /* HP */

/* Experimental two-level line tables. NOT STD DWARF5
 * Not saying GNU or anything. There are no
 * DW_LNS_lo_user or DW_LNS_hi_user values though.
 * DW_LNS_set_address_from_logical and
 * DW_LNS_set_subprogram being both 0xd
 * to avoid using up more space in the special opcode table.
 * EXPERIMENTAL DW_LNS follow.
 */
#define DW_LNS_set_address_from_logical 0x0d /* Actuals table only */
#define DW_LNS_set_subprogram           0x0d /* Logicals table only */
#define DW_LNS_inlined_call             0x0e /* Logicals table only */
#define DW_LNS_pop_context              0x0f /* Logicals table only */

#define DW_DEFSYM(NAME, VALUE) DW_CONCAT3(DW_PREFIX, _, NAME) = VALUE,
enum dwarf_symbols_at {
#define DW_PREFIX DW_AT
DW_AT_SYMBOLS(DW_DEFSYM)
#undef DW_PREFIX
};
enum dwarft_symbols_tag {
#define DW_PREFIX DW_TAG
DW_TAG_SYMBOLS(DW_DEFSYM)
#undef DW_PREFIX
};

enum dwarft_symbols_children {
#define DW_PREFIX DW_CHILDREN
DW_CHILDREN_SYMBOLS(DW_DEFSYM)
#undef DW_PREFIX
};
enum dwarft_symbols_form {
#define DW_PREFIX DW_FORM
DW_FORM_SYMBOLS(DW_DEFSYM)
#undef DW_PREFIX
};
enum dwarft_symbols_lns {
#define DW_PREFIX DW_LNS
DW_LNS_SYMBOLS(DW_DEFSYM)
#undef DW_PREFIX
};
enum dwarft_symbols_lne {
#define DW_PREFIX DW_LNE
DW_LNE_SYMBOLS(DW_DEFSYM)
#undef DW_PREFIX
};
enum dwarft_symbols_vis {
#define DW_PREFIX DW_VIS
DW_VIS_SYMBOLS(DW_DEFSYM)
#undef DW_PREFIX
};

enum dwarf_symbols_ut {
#define DW_PREFIX DW_UT
DW_UT_SYMBOLS(DW_DEFSYM)
#undef DW_PREFIX
};
#undef DW_DEFSYM

#endif /* DWELLER_SYMBOLS_H */
