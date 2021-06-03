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
#ifndef DWELLER_DWARF_H
#define DWELLER_DWARF_H
#include <stdbool.h>
#include <stddef.h>

#include <dweller/core.h>
#include <dweller/error.h>
#include <dweller/symbols.h>

/* An offset into a segment or file */
typedef dw_u64_t dw_off_t;
/* An offset into a C string */
typedef dw_off_t dw_stroff_t;
/* Type capable of representing every known DWARF symbol value */
typedef unsigned dw_symval_t;

typedef struct dwarf_debug_information_entry dwarf_die_t;
typedef struct dwarf_unit dwarf_unit_t;
typedef struct dwarf_compilation_unit dwarf_cu_t;
typedef struct dwarf_type_unit dwarf_tu_t;

typedef struct dwarf_abbreviation dwarf_abbrev_t;
typedef struct dwarf_abbreviation_attribute dwarf_abbrev_attr_t;
typedef struct dwarf_abbreviation_table dwarf_abbrev_table_t;

typedef struct dwarf_attribute dwarf_attr_t;
typedef union dwarf_attribute_value dwarf_attrval_t;

typedef struct dwarf_address_range dwarf_arange_t;
typedef struct dwarf_address_ranges dwarf_aranges_t;

enum dw_cb_status {
    DW_CB_ERR = -1, /* Fail with dwarf->errinfo */
    DW_CB_OK, /* Keep going */
    DW_CB_DONE, /* We are done. Don't call this callback anymore and return as soon as possible */
    DW_CB_NEXT, /* Don't iterate over children, go to next sibling */
    DW_CB_RESTART, /* Go back to first sibling */
};

struct dwarf;
struct dwarf_line_program;
struct dwarf_line_program_state;

typedef enum dw_cb_status (*dw_line_cb_t)(struct dwarf *dwarf, struct dwarf_line_program *program) dw_nonnull(1, 2);
typedef enum dw_cb_status (*dw_line_directory_attr_cb_t)(struct dwarf *dwarf, struct dwarf_line_program *program, dwarf_attr_t *attr) dw_nonnull(1, 2); /* TODO */
typedef enum dw_cb_status (*dw_line_file_attr_cb_t)(struct dwarf *dwarf, struct dwarf_line_program *program, dwarf_attr_t *attr) dw_nonnull(1, 2); /* TODO */
typedef enum dw_cb_status (*dw_line_row_cb_t)(struct dwarf *dwarf, struct dwarf_line_program *program, struct dwarf_line_program_state *state, struct dwarf_line_program_state *last_state) dw_nonnull(1, 2, 3, 4);
typedef enum dw_cb_status (*dw_abbrev_cb_t)(struct dwarf *dwarf, dwarf_abbrev_t *abbrev) dw_nonnull(1, 2);
typedef enum dw_cb_status (*dw_abbrev_attr_cb_t)(struct dwarf *dwarf, dwarf_abbrev_t *abbrev, dwarf_abbrev_attr_t *spec) dw_nonnull(1, 2);
typedef enum dw_cb_status (*dw_aranges_cb_t)(struct dwarf *dwarf, dwarf_aranges_t *aranges) dw_nonnull(1, 2);
typedef enum dw_cb_status (*dw_arange_cb_t)(struct dwarf *dwarf, dwarf_aranges_t *aranges, dwarf_arange_t *arange) dw_nonnull(1, 2, 3);
/* Callback that gets called when _any_ type of DIE has been parsed,
 * including unit headers. FIXME: This isn't true? We dont call on unit headers????
 */
typedef enum dw_cb_status (*dw_die_cb_t)(struct dwarf *dwarf, dwarf_unit_t *unit, dwarf_die_t *die) dw_nonnull(1, 2);
/* Callback that gets called when an attribute is found in a DIE.
 */
typedef enum dw_cb_status (*dw_die_attr_cb_t)(struct dwarf *dwarf, dwarf_unit_t *unit, dwarf_die_t *die, dwarf_attr_t *attr) dw_nonnull(1, 2, 3);
/* Callback that gets called when a `DW_UT_compile_unit` header has been parsed.
 * This is the only unit type in DWARF4 and below.
 */
typedef enum dw_cb_status (*dw_cu_cb_t)(struct dwarf *dwarf, dwarf_cu_t *cu) dw_nonnull(1, 2);
/* Callback that gets called when a `DW_UT_type_unit` header has been parsed.
 */
typedef enum dw_cb_status (*dw_tu_cb_t)(struct dwarf *dwarf, dwarf_tu_t *ud) dw_nonnull(1, 2);

#define DW_NUM_DEFAULT_OPCODES 12

/* These are all the historically used DWARF sections I could find.
 * Some got renamed (`.line -> .debug_line`), some got removed altogether.
 * Tries to follow the latest (DWARF5) naming conventions whenever possible.
 */
enum dwarf_section_namespace {
#define DWARF_SECTION_UNKNOWN 0
    DWARF_SECTION_ARANGES = 1,
    DWARF_SECTION_INFO,
    DWARF_SECTION_LINE,
    /* v Deprecated in DWARF5 (See `DWARF_SECTION_NAMES`) */
    DWARF_SECTION_PUBNAMES,
    DWARF_SECTION_STR,
    /********* DWARF2 *********/
    DWARF_SECTION_ABBREV,
    DWARF_SECTION_FRAME,
    DWARF_SECTION_MACRO,
    /* v Deprecated in DWARF5 (See `DWARF_SECTION_LOCATIONLISTS`) */
    DWARF_SECTION_LOCATIONS,
    /********* DWARF3 *********/
    /* v Deprecated in DWARF5 (See `DWARF_SECTION_RANGELISTS`) */
    DWARF_SECTION_RANGES,
    DWARF_SECTION_PUBTYPES,
    /********* DWARF4 *********/
    /* v Deprecated in DWARF5 (See `DWARF_SECTION_NAMES`) */
    DWARF_SECTION_TYPES,
    /********* DWARF5 *********/
    DWARF_SECTION_ADDR,
    DWARF_SECTION_CUINDEX,
    /* v Replaces `DWARF_SECTION_RANGES` */
    DWARF_SECTION_RANGELISTS,
    DWARF_SECTION_LINESTR,
    /* v Replaces `DWARF_SECTION_LOCATIONS` */
    DWARF_SECTION_LOCATIONLISTS,
    /* v Replaces `DWARF_SECTION_TYPES` and `DWARF_SECTION_PUBNAMES` */
    DWARF_SECTION_NAMES,
    DWARF_SECTION_STROFFSETS,
    DWARF_SECTION_SUPLEMENTARY,
    DWARF_SECTION_TUINDEX,

#define DWARF_SECTION_DWO 0x400 /* Assume any section can be DWO */
};
typedef struct dwarf_string dw_str_t;
struct dwarf_string {
    enum dwarf_section_namespace section;
    dw_stroff_t off;
    size_t len;
};
struct dwarf_section {
    const dw_u8_t *base;
    size_t size;
};
struct dwarf_section_provider {
    dw_reader_t reader;
    dw_seeker_t seeker;
    dw_mapper_t mapper;
};
struct dwarf_abbreviation {
    dw_off_t offset;
    dw_symval_t abbrev_code;
    dw_symval_t tag;
    bool has_children;
    dw_abbrev_attr_cb_t abbrev_attr_cb;
    void *data;
};
struct dwarf_abbreviation_attribute {
    dw_symval_t name;
    dw_symval_t form;
};
struct dwarf_abbreviation_table {
    dw_off_t debug_abbrev_offset;
    bool is_sorted; /* TODO: Implement bsearch if it is */
    bool is_sequential;
    size_t num_abbreviations;
    dwarf_abbrev_t *abbreviations;
};
struct dwarf_section_abbrev {
    struct dwarf_section section;
    struct dwarf_section_provider *section_provider;
    size_t num_tables;
    struct dwarf_abbreviation_table *tables;
};
struct dwarf_address_range {
    dw_u64_t segment;
    dw_u64_t base;
    dw_u64_t size;
};
struct dwarf_address_ranges {
    dw_off_t section_offset;
    size_t length; /* -1 if unknown/uncalculated */
    dw_u16_t version;
    dw_off_t debug_info_offset;
    dw_u8_t address_size;
    dw_u8_t segment_size;
    size_t num_ranges;
    struct dwarf_address_range *ranges;
    dw_u8_t dwarf64;
    size_t header_size;
    dw_arange_cb_t arange_cb;
    void *data;
};
struct dwarf_section_aranges {
    struct dwarf_section section;
    struct dwarf_section_provider *section_provider;
    size_t num_aranges;
    struct dwarf_address_ranges *aranges;
};
struct dwarf_section_info {
    struct dwarf_section section;
    struct dwarf_section_provider *section_provider;
};
struct dwarf_section_line {
    struct dwarf_section section;
    struct dwarf_section_provider *section_provider;
};
struct dwarf_section_str {
    struct dwarf_section section;
    struct dwarf_section_provider *section_provider;
};
struct dwarf_section_line_str {
    struct dwarf_section section;
    struct dwarf_section_provider *section_provider;
};

union dwarf_attribute_value {
    void *addr;
    dw_off_t off;
    dw_stroff_t stroff;
    dw_str_t str;
    dw_u64_t val;
    bool b;
    dw_u64_t file_index;
};
struct dwarf_attribute {
    dw_symval_t name;
    dw_symval_t form;
    dwarf_attrval_t value;
};

struct dwarf_pathinfo {
    dw_symval_t form;
    dwarf_attrval_t value;
};
struct dwarf_fileinfo {
    dw_str_t    name;
    size_t      include_directory_idx;
    dw_u64_t    last_modification_time;
    dw_u64_t    file_size;
};

struct dwarf_debug_information_entry {
    enum dwarf_section_namespace section;
    dw_off_t section_offset;
    size_t length; /* -1 if unknown/uncalculated */
    dw_symval_t abbrev_code; /* 0 for unit headers */
    struct dwarf_debug_information_entry *last_sibling;
    struct dwarf_debug_information_entry *next_sibling;
    struct dwarf_debug_information_entry *parent;
    dw_symval_t tag;
    bool has_children;
    int depth; /* Number of parents (-1 if unknown) */
    dw_die_attr_cb_t attr_cb;
    void *data;
};
enum dwarf_unit_type { /* These correspond to the `DW_UT_*` values */
#define DWARF_UNITTYPE_UNKNOWN 0
    DWARF_UNITTYPE_COMPILE = 0x01,
    DWARF_UNITTYPE_TYPE,
    DWARF_UNITTYPE_PARTIAL,
    DWARF_UNITTYPE_SKELETON,
    DWARF_UNITTYPE_SPLITCOMPILE,
    DWARF_UNITTYPE_SPLITTYPE,
};
struct dwarf_unit {
    struct dwarf_debug_information_entry die;
    enum dwarf_unit_type type;
    dw_off_t debug_abbrev_offset;
    dw_off_t header_size;
    struct dwarf_abbreviation_table *abbrev_table;
    dw_u8_t address_size;
    dw_u8_t dwarf64;
    dw_u16_t version;
    dw_die_cb_t die_cb;
    void *data;
};
struct dwarf_compilation_unit {
    struct dwarf_unit unit;
};
struct dwarf_type_unit {
    struct dwarf_unit unit;
};
struct dwarf_line_program_format {
    dw_u64_t name;
    dw_u64_t form;
};
struct dwarf_line_program {
    dw_off_t                            section_offset;
    size_t                              length;
    size_t                              header_size;
    dw_u8_t                             dwarf64;
    dw_u16_t                            version;
    dw_u8_t                             address_size;
    dw_u8_t                             segment_selector_size;
    size_t                              header_length;
    /* Size of instructions for target machine (1byte for x86/amd64)
     * DWARF calls this `minimum instruction length`, but that is misleading
     * as we require every instruction to have a byte length that's a
     * _multiple_ of this value.
     */
    dw_u8_t                             instruction_size;
    dw_u8_t                             maximum_operations_per_instruction;
    dw_u8_t                             default_is_stmt;
    dw_i8_t                             line_base;
    dw_u8_t                             line_range;
    dw_u8_t                             first_special_opcode;
    /* dw_u8_t num_basic_opcodes = first_special_opcode - 1; */
    dw_u8_t                            *basic_opcode_argcount;

    size_t                              num_include_directories;
    struct dwarf_pathinfo              *include_directories;
    size_t                              num_files;
    struct dwarf_fileinfo              *files;

    size_t                              directorydata_format_count;
    struct dwarf_line_program_format   *directorydata_format;
    size_t                              filedata_format_count;
    struct dwarf_line_program_format   *filedata_format;
    void                               *directorydata;
    void                               *filedata;

    size_t                              total_include_path_size;
    size_t                              total_file_path_size;
    dw_off_t                            program_offset;
    dw_line_row_cb_t                    line_row_cb;
    void                               *data;
};
#define DWARF_LINEPROG_DIRTY_ADDR           0x0001
#define DWARF_LINEPROG_DIRTY_OPINDEX        0x0002
#define DWARF_LINEPROG_DIRTY_FILE           0x0004
#define DWARF_LINEPROG_DIRTY_LINE           0x0008
#define DWARF_LINEPROG_DIRTY_COLUMN         0x0010
#define DWARF_LINEPROG_DIRTY_ISA            0x0020
#define DWARF_LINEPROG_DIRTY_DISCRIMINATOR  0x0040
#define DWARF_LINEPROG_DIRTY_ISSTMT         0x0080
#define DWARF_LINEPROG_DIRTY_BASICBLOCK     0x0100
#define DWARF_LINEPROG_DIRTY_ENDSEQ         0x0200
#define DWARF_LINEPROG_DIRTY_PROLOGUE_END   0x0400
#define DWARF_LINEPROG_DIRTY_EPILOGUE_BEGIN 0x0800
struct dwarf_line_program_state {
    dw_u64_t flags;
    dw_u64_t address;
    dw_u64_t op_index;
    dw_u64_t file;
    dw_u64_t line;
    dw_u64_t column;
    dw_u64_t isa;
    dw_u64_t discriminator;
    bool is_stmt;
    bool basic_block;
    bool end_sequence;
    bool prologue_end;
    bool epilogue_begin;
};

struct dwarf {
    struct dwarf_section_abbrev   abbrev;
    struct dwarf_section_aranges  aranges;
    struct dwarf_section_info     info;
    struct dwarf_section_line     line;
    struct dwarf_section_str      str;
    struct dwarf_section_line_str line_str;
    int                           address_size;
    struct dwarf_errinfo         *errinfo;
    dw_alloc_t                   *allocator;
    dw_die_cb_t                   die_cb;
    dw_die_attr_cb_t              attr_cb;
    dw_cu_cb_t                    cu_cb;
    dw_tu_cb_t                    tu_cb;
    dw_aranges_cb_t               aranges_cb;
    dw_arange_cb_t                arange_cb;
    dw_abbrev_cb_t                abbrev_cb;
    dw_abbrev_attr_cb_t           abbrev_attr_cb;
    dw_line_cb_t                  line_cb;
    dw_line_row_cb_t              line_row_cb;
    void                         *data;
};

DWAPI(dwarf_abbrev_t *) dwarf_abbrev_table_find_abbrev_from_code(struct dwarf *dwarf, struct dwarf_abbreviation_table *table, dw_symval_t abbrev_code);

DWAPI(bool) dwarf_write_error(const struct dwarf_errinfo *info, dw_writer_t *writer);

/* See ./tools/calc_max_symbol_size.c */
#define DWARF_MAX_SYMBOL_NAME 31

DWAPI(dw_mustuse const char *) dwarf_get_symbol_name(enum dwarf_symbol_namespace ns, dw_symval_t value);
DWAPI(dw_mustuse const char *) dwarf_get_symbol_shortname(enum dwarf_symbol_namespace ns, dw_symval_t value);

DWAPI(bool) dwarf_init(struct dwarf **dwarf, dw_alloc_t *allocator, struct dwarf_errinfo *errinfo) dw_nonnull(1);
DWAPI(void) dwarf_fini(struct dwarf **dwarf, const struct dwarf_errinfo *errinfo) dw_nonnull(1);

/* Load a section of DWARF data already read into memory.
 * This decreases the number of allocations that `libdweller` has to do.
 * The section data must remain valid while it is loaded, of course...
 * (don't `munmap` or `free` it!)
 */
DWAPI(bool) dwarf_load_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_section section, struct dwarf_errinfo *errinfo) dw_nonnull(1);

/**
 * Add section of DWARF data giving a provider.
 * The data is read lazily, as it is needed.
 */
DWAPI(bool) dwarf_add_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_section_provider *provider, struct dwarf_errinfo *errinfo) dw_nonnull(1);

/**
 * Returns true if the given section is loaded.
 */
DWAPI(bool) dwarf_has_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_errinfo *errinfo) dw_nonnull(1);

typedef struct dwarf_iter dwarf_iter_t;
typedef struct dwarf_aranges_iter dwarf_aranges_iter_t;
typedef struct dwarf_arange_iter dwarf_arange_iter_t;
typedef struct dwarf_unit_iter dwarf_unit_iter_t;
typedef struct dwarf_die_iter dwarf_die_iter_t;
typedef struct dwarf_attr_iter dwarf_attr_iter_t;
typedef struct dwarf_line_program_iter dwarf_line_program_iter_t;
typedef struct dwarf_line_row_iter dwarf_line_row_iter_t;

struct dwarf_iter {
    void *(*next)(dwarf_iter_t *iter);
    struct dwarf *dwarf;
    struct dwarf_errinfo *errinfo;
};

DWAPI(void *) dwarf_iter_next(dwarf_iter_t *iter);

#define dwarf_next(self) dwarf_iter_next((dwarf_iter_t *)(self))

DWAPI(bool) dwarf_aranges_iter_init(struct dwarf *dwarf, dwarf_aranges_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_arange_iter_init(struct dwarf *dwarf, dwarf_arange_iter_t *iter, dwarf_aranges_t *aranges, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_unit_iter_init(struct dwarf *dwarf, dwarf_unit_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_die_iter_init(struct dwarf *dwarf, dwarf_die_iter_t *iter, dwarf_unit_t *unit, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_attr_iter_init(struct dwarf *dwarf, dwarf_attr_iter_t *iter, dwarf_unit_t *unit, dwarf_die_t *die, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_attr_iter_from(struct dwarf *dwarf, dwarf_attr_iter_t *iter, dwarf_die_iter_t *die_iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_line_program_iter_init(struct dwarf *dwarf, dwarf_line_program_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_line_row_iter_init(struct dwarf *dwarf, dwarf_line_row_iter_t *iter, struct dwarf_line_program *line_program, struct dwarf_errinfo *errinfo);

DWAPI(bool) dwarf_arange_iter_fini(struct dwarf *dwarf, dwarf_arange_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_aranges_iter_fini(struct dwarf *dwarf, dwarf_aranges_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_unit_iter_fini(struct dwarf *dwarf, dwarf_unit_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_die_iter_fini(struct dwarf *dwarf, dwarf_die_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_attr_iter_fini(struct dwarf *dwarf, dwarf_attr_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_line_program_iter_fini(struct dwarf *dwarf, dwarf_line_program_iter_t *iter, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_line_row_iter_fini(struct dwarf *dwarf, dwarf_line_row_iter_t *iter, struct dwarf_errinfo *errinfo);

DWAPI(dwarf_aranges_iter_t *)      dwarf_aranges_iter(struct dwarf *dwarf, dwarf_aranges_iter_t *saved, struct dwarf_errinfo *errinfo);
DWAPI(dwarf_arange_iter_t *)       dwarf_arange_iter(struct dwarf *dwarf, dwarf_arange_iter_t *saved, dwarf_aranges_t *aranges, struct dwarf_errinfo *errinfo);
DWAPI(dwarf_unit_iter_t *)         dwarf_unit_iter(struct dwarf *dwarf, dwarf_unit_iter_t *saved, struct dwarf_errinfo *errinfo);
DWAPI(dwarf_die_iter_t *)          dwarf_unit_entry_iter(struct dwarf *dwarf, dwarf_die_iter_t *saved, dwarf_unit_t *unit, struct dwarf_errinfo *errinfo);
DWAPI(dwarf_attr_iter_t *)         dwarf_unit_entry_attr_iter(struct dwarf *dwarf, dwarf_attr_iter_t *saved, dwarf_die_iter_t *die_iter, struct dwarf_errinfo *errinfo);
DWAPI(dwarf_attr_iter_t *)         dwarf_die_attr_iter(struct dwarf *dwarf, dwarf_attr_iter_t *saved, dwarf_unit_t *unit, dwarf_die_t *die, struct dwarf_errinfo *errinfo);
DWAPI(dwarf_line_program_iter_t *) dwarf_line_program_iter(struct dwarf *dwarf, dwarf_line_program_iter_t *saved, struct dwarf_errinfo *errinfo);
DWAPI(dwarf_line_row_iter_t *)     dwarf_line_row_iter(struct dwarf *dwarf, dwarf_line_row_iter_t *saved, struct dwarf_line_program *program, struct dwarf_errinfo *errinfo);

DWAPI(void) dwarf_aranges_iter_free(struct dwarf *dwarf, dwarf_aranges_iter_t *iter);
DWAPI(void) dwarf_arange_iter_free(struct dwarf *dwarf, dwarf_arange_iter_t *iter);
DWAPI(void) dwarf_unit_iter_free(struct dwarf *dwarf, dwarf_unit_iter_t *iter);
DWAPI(void) dwarf_die_iter_free(struct dwarf *dwarf, dwarf_die_iter_t *iter);
DWAPI(void) dwarf_attr_iter_free(struct dwarf *dwarf, dwarf_attr_iter_t *iter);
DWAPI(void) dwarf_line_program_iter_free(struct dwarf *dwarf, dwarf_line_program_iter_t *iter);
DWAPI(void) dwarf_line_row_iter_free(struct dwarf *dwarf, dwarf_line_row_iter_t *iter);

DWAPI(bool) dwarf_aranges_at(struct dwarf *dwarf, dwarf_aranges_t *aranges, dw_u64_t off, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_arange_at(struct dwarf *dwarf, dwarf_aranges_t *aranges, dwarf_arange_t *arange, dw_u64_t off, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_unit_at(struct dwarf *dwarf, dwarf_unit_t *unit, dw_u64_t off, struct dwarf_errinfo *errinfo);
DWAPI(bool) dwarf_die_at(struct dwarf *dwarf, dwarf_unit_t *unit, dwarf_die_t *die, dw_u64_t off, struct dwarf_errinfo *errinfo);

// DWAPI(bool) dwarf_parse_die_at(struct dwarf *dwarf, dwarf_unit_t *unit, dw_i64_t *off, struct dwarf_errinfo *errinfo) dw_nonnull(1);

/* Parse a section.
 * Once a relevant piece of information like a DIE or CU has been detected,
 * invokes corresponding callbacks.
 * An already parsed section will simply execute `dwarf_walk_section`.
 */
DWAPI(bool) dwarf_parse_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_errinfo *errinfo) dw_nonnull(1);
/* Parse _all_ sections */
DWAPI(bool) dwarf_parse(struct dwarf *dwarf, struct dwarf_errinfo *errinfo) dw_nonnull(1);

/* 'Walk' a section. That is, parse it without memoizing anything.
 * This is much faster and consumes less memory, but is very inefficient when
 * walking large sections of debug data more than once.
 * Callbacks might also not receive their arguments fully filled in.
 */
DWAPI(bool) dwarf_walk_section(struct dwarf *dwarf, enum dwarf_section_namespace ns, struct dwarf_errinfo *errinfo) dw_nonnull(1);
/* Walk _all_ sections */
DWAPI(bool) dwarf_walk(struct dwarf *dwarf, struct dwarf_errinfo *errinfo) dw_nonnull(1);

#endif /* DWELLER_DWARF_H */
