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
#include <dweller/libc.h>
#include <dweller/elf.h>

#include <stdint.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void loadelf(struct dwarf *dwarf, const uint8_t *data, size_t size, struct dwarf_errinfo *errinfo);

static void printusage()
{
    puts("USAGE: dwarfdump <object file>");
}

static void indent(int n)
{
    for (; n; n--) printf(" ");
}

static enum dw_cb_status line_row_cb(struct dwarf *dwarf, struct dwarf_line_program *program, struct dwarf_line_program_state *state, struct dwarf_line_program_state *last_state)
{
    printf("0x%08zx  [%4u, %u]", state->address, state->line, state->column);
    if (state->is_stmt) printf(" NS");
    if (state->basic_block) printf(" BB");
    if (state->end_sequence) printf(" ET");
    if (state->prologue_end) printf(" PE");
    if (state->epilogue_begin) printf(" EB");
    if (state->isa != last_state->isa) printf(" IS=%x", state->isa);
    if (state->discriminator) printf(" DI=%#x", state->discriminator);
    if (state->file != last_state->file) {
        const char *path = dwarf->line.section.base + program->files[state->file - 1].name; /* FIXME: Check that state->file is actually valid and not out of bounds */
        printf(" uri: \"%s\"", path); /* FIXME: String escapes? */
    }
    printf("\n");
    return DW_CB_OK;
}
static enum dw_cb_status line_cb(struct dwarf *dwarf, struct dwarf_line_program *program)
{
    size_t i;
    printf(" Opcodes:\n");
    for (i=0; i < program->num_basic_opcodes - 1; i++) {
        printf("  Opcode 0x%02x has %u argument(s)\n", i + 1, program->basic_opcode_argcount[i]);
    }
    printf("\n");
    printf(" The Include Directory Table\n");
    for (i=0; i < program->num_include_directories; i++) {
        const char *path = dwarf->line.section.base + program->include_directories[i];
        printf("  %zu\t%s\n", i, path);
    }
    printf("\n");
    printf(" The File Information Table\n");
    printf("  Entry\tDir\tTime\tSize\tName\n");
    for (i=0; i < program->num_files; i++) {
        struct dwarf_fileinfo *info = &program->files[i];
        const char *path = dwarf->line.section.base + info->name;
        printf("  %zu\t", i + 1);
        printf("%zu\t", info->include_directory_idx);
        printf("%zd\t", info->last_modification_time);
        printf("%zd\t", info->file_size);
        printf("%s\n", path);
    }
    printf("\n");
    program->line_row_cb = line_row_cb;
    return DW_CB_OK;
}
static enum dw_cb_status abbrev_cb(struct dwarf *dwarf, dwarf_abbrev_t *abbrev)
{
    const char *tagname = dwarf_get_symbol_name(DW_TAG, abbrev->tag);
    printf("Defining abid 0x%02x as tag %s (0x%02x) [%s]\n", abbrev->abbrev_code, tagname, abbrev->tag, abbrev->has_children ? "has children" : "no children");
    return DW_CB_OK;
}
static enum dw_cb_status abbrev_attr_cb(struct dwarf *dwarf, dwarf_abbrev_t *abbrev, dwarf_abbrev_attr_t *attr)
{
    const char *attr_name = dwarf_get_symbol_name(DW_AT, attr->name);
    const char *form_name = dwarf_get_symbol_name(DW_FORM, attr->form);
    printf("  %s (0x%02x): %s (0x%02x)\n", attr_name, attr->name, form_name, attr->form);
    return DW_CB_OK;
}

static enum dw_cb_status arange_cb(struct dwarf *dwarf, dwarf_aranges_t *aranges, dwarf_arange_t *arange)
{
    printf("  [%lu] 0x%016lx - 0x%016lx | 0x%06lx (%zu bytes)\n", arange->segment, arange->base, arange->base + arange->size, arange->size, arange->size);
    return DW_CB_OK;
}
static enum dw_cb_status aranges_cb(struct dwarf *dwarf, dwarf_aranges_t *aranges)
{
    printf("  Length:                   %zu\n", aranges->length);
    printf("  Version:                  %zu\n", aranges->version);
    printf("  Offset:                   0x%08zx\n", aranges->debug_info_offset);
    printf("  Pointer Size:             0x%08zx\n", aranges->address_size);
    printf("  Segment Size:             0x%08zx\n", aranges->segment_size);
    printf("\n");
    printf("  Seg From                 To                   Length\n");
    aranges->arange_cb = arange_cb;
    return DW_CB_OK;
}
static enum dw_cb_status attr_cb(struct dwarf *dwarf, dwarf_die_t *die, dwarf_attr_t *attr)
{
    const char *attr_name = dwarf_get_symbol_name(DW_AT, attr->name);
    const char *form_name = dwarf_get_symbol_name(DW_FORM, attr->form);
    indent(die->depth * 2);
    printf("  [%s] = ", attr_name);
    switch (attr->form) {
    case DW_FORM_flag_present:
        printf("true (implicit)");
        break;
    case DW_FORM_flag:
        printf("%s", attr->value.b ? "true" : "false");
        break;
    case DW_FORM_data1:
    case DW_FORM_ref1:
    case DW_FORM_data2:
    case DW_FORM_ref2:
    case DW_FORM_data4:
    case DW_FORM_ref4:
    case DW_FORM_data8:
    case DW_FORM_ref8:
        printf("0x%02x", attr->value.val);
        break;
    case DW_FORM_addr:
        printf("%p", attr->value.addr);
        break;
    case DW_FORM_sec_offset:
        printf("0x%02x", attr->value.off);
        break;
    case DW_FORM_string:
        printf("'%s'", attr->value.cstr);
        break;
    case DW_FORM_strp:
        {
            const char *str = dwarf->str.section.base + attr->value.stroff;
            printf("'%s' (offset %#02x)", str, attr->value.stroff);
        }
        break;
    case DW_FORM_exprloc:
        { /* TODO */
            printf("???");
        }
        break;
    }
    printf(" # %s (0x%02x): %s (0x%02x)\n", attr_name, attr->name, form_name, attr->form);
    return DW_CB_OK;
}
static enum dw_cb_status die_cb(struct dwarf *dwarf, dwarf_die_t *die)
{
    dwarf_abbrev_t *abbrev = dwarf_abbrev_table_find_abbrev_from_code(dwarf, die->abbrev_table, die->abbrev_code);
    indent(die->depth * 2); printf("<%x>\n", die->section_offset);
    indent(die->depth * 2); printf("@abid: 0x%02x\n", die->abbrev_code);
    indent(die->depth * 2); printf("@offset: 0x%x\n", abbrev->offset);
    indent(die->depth * 2); printf("@depth: %d\n", die->depth);
    indent(die->depth * 2); printf("@tag: %s%s\n", dwarf_get_symbol_name(DW_TAG, die->tag), die->has_children ? " [has children]" : " [no children]");
    die->attr_cb = attr_cb;
    return DW_CB_OK;
}

static const uint8_t *mapfile(const char *filename, size_t *size) {
    uint8_t *data = MAP_FAILED;
    int fd = open(filename, O_RDONLY);
    if (fd == -1) goto fail;
    struct stat sb;
    if (fstat(fd, &sb) == -1) goto fail;
    data = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) goto fail;
    if (size) *size = sb.st_size;

    if (fd != -1) close(fd);
    return data;

fail:
    if (data != MAP_FAILED) munmap(data, sb.st_size);
    if (fd != -1) close(fd);
    return data;
}
static void unmapfile(const uint8_t *data, size_t size) {
    if (data != MAP_FAILED) munmap((void *)data, size);
}

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        printusage();
        exit(1);
    }
    struct dwarf *dwarf;
    struct dwarf_errinfo errinfo = DWARF_ERRINFO_INIT;
    dwarf_init(&dwarf, &dweller_libc_allocator, &errinfo);
    dwarf->line_cb = line_cb;
    dwarf->aranges_cb = aranges_cb;
    dwarf->die_cb = die_cb;
    size_t size = 0;
    uint8_t *data = mapfile(argv[1], &size);
    loadelf(dwarf, data, size, &errinfo);
    if (data == MAP_FAILED) goto fail;
    dwarf_parse(dwarf, &errinfo);

fail:
    if (data == MAP_FAILED) perror(argv[1]);
    unmapfile(data, size);
    if (dwarf_has_error(&errinfo)) {
        dwarf_write_error(&errinfo, &dweller_libc_stderr_writer);
    }
    dwarf_fini(&dwarf, &errinfo);
    return 0;
}

static void loadelf32(struct dwarf *dwarf, const uint8_t *data, size_t size, struct dwarf_errinfo *errinfo)
{
    Elf32_Ehdr *ehdr = data;
    Elf32_Shdr *strh = data + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize);
    char *strs = data + strh->sh_offset;
    printf("num sections: %d\n", ehdr->e_shnum);
    Elf32_Half i;
    for (i=0; i < ehdr->e_shnum; i++) {
        Elf32_Shdr *shdr = data + ehdr->e_shoff + (i * ehdr->e_shentsize);
        char *name = &strs[shdr->sh_name];
        printf("0x%016x:", shdr->sh_offset);
        printf("%#8x:", shdr->sh_type);
        printf("'%s'", name);
        printf("\n");
        struct dwarf_section section;
        section.base = data + shdr->sh_offset;
        section.size = shdr->sh_size;
        if      (strcmp(name, ".debug_abbrev") == 0) dwarf_load_section(dwarf, DWARF_SECTION_ABBREV, section, errinfo);
        else if (strcmp(name, ".debug_aranges") == 0) dwarf_load_section(dwarf, DWARF_SECTION_ARANGES, section, errinfo);
        else if (strcmp(name, ".debug_info") == 0) dwarf_load_section(dwarf, DWARF_SECTION_INFO, section, errinfo);
        else if (strcmp(name, ".debug_line") == 0) dwarf_load_section(dwarf, DWARF_SECTION_LINE, section, errinfo);
        else if (strcmp(name, ".debug_str") == 0) dwarf_load_section(dwarf, DWARF_SECTION_STR, section, errinfo);
    }
}
static void loadelf64(struct dwarf *dwarf, const uint8_t *data, size_t size, struct dwarf_errinfo *errinfo)
{
    Elf64_Ehdr *ehdr = data;
    Elf64_Shdr *strh = data + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize);
    char *strs = data + strh->sh_offset;
    printf("num sections: %d\n", ehdr->e_shnum);
    Elf64_Half i;
    for (i=0; i < ehdr->e_shnum; i++) {
        Elf64_Shdr *shdr = data + ehdr->e_shoff + (i * ehdr->e_shentsize);
        char *name = &strs[shdr->sh_name];
        printf("0x%016x:", shdr->sh_offset);
        printf("%#8x:", shdr->sh_type);
        printf("'%s'", name);
        printf("\n");
        struct dwarf_section section;
        section.base = data + shdr->sh_offset;
        section.size = shdr->sh_size;
        if      (strcmp(name, ".debug_abbrev") == 0) dwarf_load_section(dwarf, DWARF_SECTION_ABBREV, section, errinfo);
        else if (strcmp(name, ".debug_aranges") == 0) dwarf_load_section(dwarf, DWARF_SECTION_ARANGES, section, errinfo);
        else if (strcmp(name, ".debug_info") == 0) dwarf_load_section(dwarf, DWARF_SECTION_INFO, section, errinfo);
        else if (strcmp(name, ".debug_line") == 0) dwarf_load_section(dwarf, DWARF_SECTION_LINE, section, errinfo);
        else if (strcmp(name, ".debug_str") == 0) dwarf_load_section(dwarf, DWARF_SECTION_STR, section, errinfo);
    }
}
static void loadelf(struct dwarf *dwarf, const uint8_t *data, size_t size, struct dwarf_errinfo *errinfo)
{
    Elf_Ehdr *ehdr = data;
    if (ehdr->e_ident[EI_CLASS] == ELFCLASS32) loadelf32(dwarf, data, size, errinfo);
    if (ehdr->e_ident[EI_CLASS] == ELFCLASS64) loadelf64(dwarf, data, size, errinfo);
}
