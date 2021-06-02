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
#ifndef DWELLER_SYMBOLS_EXT_H
#define DWELLER_SYMBOLS_EXT_H

/* SGI/MIPS extensions.  */
#define DW_AT_MIPS_fde 0x2001
#define DW_AT_MIPS_loop_begin 0x2002
#define DW_AT_MIPS_tail_loop_begin 0x2003
#define DW_AT_MIPS_epilog_begin 0x2004
#define DW_AT_MIPS_loop_unroll_factor 0x2005
#define DW_AT_MIPS_software_pipeline_depth 0x2006
#define DW_AT_MIPS_linkage_name 0x2007
#define DW_AT_MIPS_stride 0x2008
#define DW_AT_MIPS_abstract_name 0x2009
#define DW_AT_MIPS_clone_origin 0x200a
#define DW_AT_MIPS_has_inlines 0x200b
/* HP extensions.  */
#define DW_AT_HP_block_index 0x2000
#define DW_AT_HP_unmodifiable 0x2001 /* Same as DW_AT_MIPS_fde.  */
#define DW_AT_HP_prologue 0x2005 /* Same as DW_AT_MIPS_loop_unroll.  */
#define DW_AT_HP_epilogue 0x2008 /* Same as DW_AT_MIPS_stride.  */
#define DW_AT_HP_actuals_stmt_list 0x2010
#define DW_AT_HP_proc_per_section 0x2011
#define DW_AT_HP_raw_data_ptr 0x2012
#define DW_AT_HP_pass_by_reference 0x2013
#define DW_AT_HP_opt_level 0x2014
#define DW_AT_HP_prof_version_id 0x2015
#define DW_AT_HP_opt_flags 0x2016
#define DW_AT_HP_cold_region_low_pc 0x2017
#define DW_AT_HP_cold_region_high_pc 0x2018
#define DW_AT_HP_all_variables_modifiable 0x2019
#define DW_AT_HP_linkage_name 0x201a
#define DW_AT_HP_prof_flags 0x201b  /* In comp unit of procs_info for -g.  */
#define DW_AT_HP_unit_name 0x201f
#define DW_AT_HP_unit_size 0x2020
#define DW_AT_HP_widened_byte_size 0x2021
#define DW_AT_HP_definition_points 0x2022
#define DW_AT_HP_default_location 0x2023
#define DW_AT_HP_is_result_param 0x2029
/* GNU extensions.  */
#define DW_AT_sf_names 0x2101
#define DW_AT_src_info 0x2102
#define DW_AT_mac_info 0x2103
#define DW_AT_src_coords 0x2104
#define DW_AT_body_begin 0x2105
#define DW_AT_body_end 0x2106
#define DW_AT_GNU_vector 0x2107
/* Thread-safety annotations.
   See http://gcc.gnu.org/wiki/ThreadSafetyAnnotation .  */
#define DW_AT_GNU_guarded_by 0x2108
#define DW_AT_GNU_pt_guarded_by 0x2109
#define DW_AT_GNU_guarded 0x210a
#define DW_AT_GNU_pt_guarded 0x210b
#define DW_AT_GNU_locks_excluded 0x210c
#define DW_AT_GNU_exclusive_locks_required 0x210d
#define DW_AT_GNU_shared_locks_required 0x210e
/* One-definition rule violation detection.
   See http://gcc.gnu.org/wiki/DwarfSeparateTypeInfo .  */
#define DW_AT_GNU_odr_signature 0x210f
/* Template template argument name.
   See http://gcc.gnu.org/wiki/TemplateParmsDwarf .  */
#define DW_AT_GNU_template_name 0x2110
/* The GNU call site extension.
   See http://www.dwarfstd.org/ShowIssue.php?issue=100909.2&type=open .  */
#define DW_AT_GNU_call_site_value 0x2111
#define DW_AT_GNU_call_site_data_value 0x2112
#define DW_AT_GNU_call_site_target 0x2113
#define DW_AT_GNU_call_site_target_clobbered 0x2114
#define DW_AT_GNU_tail_call 0x2115
#define DW_AT_GNU_all_tail_call_sites 0x2116
#define DW_AT_GNU_all_call_sites 0x2117
#define DW_AT_GNU_all_source_call_sites 0x2118
/* Section offset into .debug_macro section.  */
#define DW_AT_GNU_macros 0x2119
/* Attribute for C++ deleted special member functions (= delete;).  */
#define DW_AT_GNU_deleted 0x211a
/* Extensions for Fission.  See http://gcc.gnu.org/wiki/DebugFission.  */
#define DW_AT_GNU_dwo_name 0x2130
#define DW_AT_GNU_dwo_id 0x2131
#define DW_AT_GNU_ranges_base 0x2132
#define DW_AT_GNU_addr_base 0x2133
#define DW_AT_GNU_pubnames 0x2134
#define DW_AT_GNU_pubtypes 0x2135
/* Attribute for discriminator.
   See http://gcc.gnu.org/wiki/Discriminator  */
#define DW_AT_GNU_discriminator 0x2136
#define DW_AT_GNU_locviews 0x2137
#define DW_AT_GNU_entry_view 0x2138
/* VMS extensions.  */
#define DW_AT_VMS_rtnbeg_pd_address 0x2201
/* GNAT extensions.  */
/* GNAT descriptive type.
   See http://gcc.gnu.org/wiki/DW_AT_GNAT_descriptive_type .  */
#define DW_AT_use_GNAT_descriptive_type 0x2301
#define DW_AT_GNAT_descriptive_type 0x2302
/* Rational constant extension.
   See https://gcc.gnu.org/wiki/DW_AT_GNU_numerator_denominator .  */
#define DW_AT_GNU_numerator 0x2303
#define DW_AT_GNU_denominator 0x2304
/* Biased integer extension.
   See https://gcc.gnu.org/wiki/DW_AT_GNU_bias .  */
#define DW_AT_GNU_bias 0x2305
/* UPC extension.  */
#define DW_AT_upc_threads_scaled 0x3210
/* PGI (STMicroelectronics) extensions.  */
#define DW_AT_PGI_lbase 0x3a00
#define DW_AT_PGI_soffset 0x3a01
#define DW_AT_PGI_lstride 0x3a02
/* Apple extensions.  */
#define DW_AT_APPLE_optimized 0x3fe1
#define DW_AT_APPLE_flags 0x3fe2
#define DW_AT_APPLE_isa 0x3fe3
#define DW_AT_APPLE_block 0x3fe4
#define DW_AT_APPLE_major_runtime_vers 0x3fe5
#define DW_AT_APPLE_runtime_class 0x3fe6
#define DW_AT_APPLE_omit_frame_ptr 0x3fe7
#define DW_AT_APPLE_property_name 0x3fe8
#define DW_AT_APPLE_property_getter 0x3fe9
#define DW_AT_APPLE_property_setter 0x3fea
#define DW_AT_APPLE_property_attribute 0x3feb
#define DW_AT_APPLE_objc_complete_type 0x3fec
#define DW_AT_APPLE_property 0x3fed



#define DW_FORM_GNU_addr_index 0x1f01 /* GNU extension in debug_info.dwo.*/
#define DW_FORM_GNU_str_index  0x1f02 /* GNU extension, somewhat like DW_FORM_strp */
#define DW_FORM_GNU_ref_alt    0x1f20 /* GNU extension. Offset in .debug_info. */
#define DW_FORM_GNU_strp_alt   0x1f21 /* GNU extension. Offset in .debug_str of another object file. */



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

#endif /* DWELLER_SYMBOLS_EXT_H */
