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

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define STRLEN(str) (sizeof(str) - 1)
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(*arr))

static void loadelf(struct dwarf *dwarf, const uint8_t *data, size_t size, struct dwarf_errinfo *errinfo);

static void error(const char *str)
{
    fprintf(stderr, "%s\n", str);
    exit(1);
}
static void printusage()
{
    puts("USAGE: dwarfdump <object file>");
}

/* Static buffer shared between all printers
 * ( To race against Gimli c: )
 */
static char buffer[4096 * 4096];
static size_t buffersz = 0;

#define output(data, size) do { if (buffersz >= 4096 * 4096 - 4096 * 2) flushoutput(data, size); } while (0)
static void flushoutput(const char *data, size_t size)
{
    int nwritten = 0;
    do {
        int res = write(STDOUT_FILENO, data, size);
        if (res == -1) {
            if (errno == EINTR || errno == EAGAIN) continue;
            abort();
        }
        nwritten += res;
    } while (nwritten < size);
    buffersz = 0;
}
static bool checkquota(size_t n) {
    return n < ARRAYSIZE(buffer) - buffersz;
}
static void ensurequota(size_t n) {
    if (!checkquota(n)) {
        flushoutput(buffer, buffersz);
        if (!checkquota(n)) error("out of memory");
    }
}

#define putlit(str) \
    do { \
        size_t i_; \
        for (i_=0; i_ < STRLEN(str); i_++) { \
            put(str[i_]); \
        } \
    } while (0)
static void put(char c) {
    buffer[buffersz++] = c;
}
static void putcstr(const char *str) {
    for (; *str; str++) put(*str);
}
static void putindent(int n) {
    for (; n; n--) put(' ');
}
static void putint(uint64_t val, int padleft) {
    char strbuf[20]; /* ceil(log10(2 ** 64)) = 20 */
    size_t strsz = 0;
    char *str = strbuf + ARRAYSIZE(strbuf);
    do {
        *--str = '0' + (val % 10);
        strsz++;
        val /= 10;
    } while (val);
    padleft -= strsz;
    while (padleft-- > 0) buffer[buffersz++] = ' ';
    memcpy(buffer + buffersz, str, strsz);
    buffersz += strsz;
}
static void puthex(uint64_t val, int padleft) {
    char strbuf[16]; /* ceil(log16(2 ** 64)) = 16 */
    size_t strsz = 0;
    char *str = strbuf + ARRAYSIZE(strbuf);
    putlit("0x");
    do {
        *--str = "0123456789abcdef"[val % 16];
        strsz++;
        val /= 16;
    } while (val);
    padleft -= strsz;
    while (padleft-- > 0) buffer[buffersz++] = ' ';
    memcpy(buffer + buffersz, str, strsz);
    buffersz += strsz;
}
static void puthex2(uint64_t val, int padleft) {
    char strbuf[16]; /* ceil(log16(2 ** 64)) = 16 */
    size_t strsz = 0;
    char *str = strbuf + ARRAYSIZE(strbuf);
    putlit("0x");
    do {
        *--str = "0123456789abcdef"[val % 16];
        strsz++;
        val /= 16;
    } while (val);
    if (strsz % 2 != 0) {
        *--str = '0';
        strsz++;
    }
    padleft -= strsz;
    while (padleft-- > 0) buffer[buffersz++] = ' ';
    memcpy(buffer + buffersz, str, strsz);
    buffersz += strsz;
}
static void putaddr32(uint64_t val) {
    int i;
    putlit("0x");
    for (i=0; i < 8; i++) {
        int hexdgt = (val >> (64 - i*4 - 4)) & 0xf;
        put("0123456789abcdef"[hexdgt]);
    }
}
static void putaddr64(uint64_t val) {
    int i;
    putlit("0x");
    for (i=0; i < 16; i++) {
        int hexdgt = (val >> (64 - i*4 - 4)) & 0xf;
        put("0123456789abcdef"[hexdgt]);
    }
}
#define putaddr(val) putaddr32(val) /* FIXME: Figure this out depending on DWARF32 or DWARF64 */
static void puturi(const char *path) {
    /* TODO */
    /* TODO: Also do bounds checking on buffersz? */
    putcstr(path);
}

static enum dw_cb_status line_row_cb(struct dwarf *dwarf, struct dwarf_line_program *program, struct dwarf_line_program_state *state, struct dwarf_line_program_state *last_state)
{
    const size_t maxn = STRLEN(
        "0xffffffffffffffff [2147483647, 2147483647] NS BB ET PE EB IS=0xffffffffffffffff DI=0xffffffffffffffff uri: \"\"\n"
    ) + program->files[state->file - 1].namesz;
    ensurequota(maxn);
    putaddr(state->address);
    putlit(" [");
    putint(state->line, 4);
    putlit(", ");
    putint(state->column, 0);
    putlit("]");
    if (state->is_stmt) putlit(" NS");
    if (state->basic_block) putlit(" BB");
    if (state->end_sequence) putlit(" ET");
    if (state->prologue_end) putlit(" PE");
    if (state->epilogue_begin) putlit(" EB");
    if (state->isa != last_state->isa) { putlit(" IS="); puthex(state->isa, 0); }
    if (state->discriminator) { putlit(" DI="); puthex(state->discriminator, 0); }
    if (state->file != last_state->file) {
        const char *path = (const char *)dwarf->line.section.base + program->files[state->file - 1].name; /* FIXME: Check that state->file is actually valid and not out of bounds */
        putlit(" uri: \"");
        puturi(path);
        put('\"');
    }
    put('\n');
    return DW_CB_OK;
}
static enum dw_cb_status line_cb(struct dwarf *dwarf, struct dwarf_line_program *program)
{
    const size_t maxn =
        STRLEN(" Opcodes:\n")
      + STRLEN("  Opcode 0xff has 255 argument(s)\n") * 255
      + STRLEN("\n")
      + STRLEN(" The Include Directory Table\n")
      + program->num_include_directories * STRLEN("  2147483647\t\n")
      + program->num_include_directories * program->total_include_path_size
      + STRLEN("\n")
      + STRLEN(" The File Information Table\n")
      + STRLEN("  Entry\tDir\tTime\tSize\tName\n")
      + program->num_files * STRLEN(
            "  2147483647\t2147483647\t2147483647\t2147483647\t\n"
        )
      + program->num_files * program->total_file_path_size
      + STRLEN("\n");
    ensurequota(maxn);
    size_t i;
    putlit(" Opcodes:\n");
    for (i=1; i < program->first_special_opcode; i++) {
        putlit("  Opcode ");
        puthex2(i, 0);
        putlit(" has ");
        putint(program->basic_opcode_argcount[i-1], 0);
        putlit(" argument(s)\n");
    }
    put('\n');
    putlit(" The Include Directory Table\n");
    for (i=0; i < program->num_include_directories; i++) {
        const char *path = (const char *)dwarf->line.section.base + program->include_directories[i];
        putlit("  ");
        putint(i, 0);
        put('\t');
        puturi(path);
        put('\n');
    }
    put('\n');
    putlit(" The File Information Table\n");
    putlit("  Entry\tDir\tTime\tSize\tName\n");
    for (i=0; i < program->num_files; i++) {
        struct dwarf_fileinfo *info = &program->files[i];
        const char *path = (const char *)dwarf->line.section.base + info->name;
        putlit("  ");
        putint(i + 1, 0);
        put('\t');
        putint(info->include_directory_idx, 0);
        put('\t');
        putint(info->last_modification_time, 0);
        put('\t');
        putint(info->file_size, 0);
        put('\t');
        puturi(path);
        put('\n');
    }
    put('\n');

    program->line_row_cb = line_row_cb;
    return DW_CB_OK;
}
static enum dw_cb_status abbrev_cb(struct dwarf *dwarf, dwarf_abbrev_t *abbrev)
{
    const size_t maxn = STRLEN(
        "Defining abbreviation code 0xffffffffffffffff as tag  [has children]\n"
    ) + MAX(STRLEN("<0xffffffffffffffff>"), DWARF_MAX_SYMBOL_NAME);
    if (!checkquota(maxn)) {
        flushoutput(buffer, buffersz);
        /* No need to check again (maxn is constant) */
    }
    const char *tag_name = dwarf_get_symbol_name(DW_TAG, abbrev->tag);
    putlit("Defining abbreviation code ");
    puthex2(abbrev->abbrev_code, 0);
    putlit(" as tag ");
    if (tag_name) {
        putcstr(tag_name);
    } else {
        put('<');
        puthex2(abbrev->tag, 0);
        put('>');
    }
    if (abbrev->has_children) {
        putlit(" [has children]");
    } else {
        putlit(" [no children]");
    }
    put('\n');

    return DW_CB_OK;
}
static enum dw_cb_status abbrev_attr_cb(struct dwarf *dwarf, dwarf_abbrev_t *abbrev, dwarf_abbrev_attr_t *attr)
{
    const size_t maxn = STRLEN(
        "  : \n"
    ) + MAX(STRLEN("<0xffffffffffffffff>"), DWARF_MAX_SYMBOL_NAME) * 2;
    if (!checkquota(maxn)) {
        flushoutput(buffer, buffersz);
        /* No need to check again (maxn is constant) */
    }
    const char *attr_name = dwarf_get_symbol_name(DW_AT, attr->name);
    const char *form_name = dwarf_get_symbol_name(DW_FORM, attr->form);
    putlit("  ");
    if (attr_name) {
        putcstr(attr_name);
    } else {
        put('<');
        puthex2(attr->name, 0);
        put('>');
    }
    putlit(": ");
    if (form_name) {
        putcstr(form_name);
    } else {
        put('<');
        puthex2(attr->form, 0);
        put('>');
    }
    put('\n');

    return DW_CB_OK;
}

static enum dw_cb_status arange_cb(struct dwarf *dwarf, dwarf_aranges_t *aranges, dwarf_arange_t *arange)
{
    const size_t maxn = STRLEN("  [2147483647] 0xffffffffffffffff - 0xffffffffffffffff | 0xffffffffffffffff (2147483647 bytes)\n");
    if (!checkquota(maxn)) {
        flushoutput(buffer, buffersz);
        /* No need to check again (maxn is constant) */
    }
    putlit("  [");
    putint(arange->segment, 0);
    putlit("] ");
    putaddr64(arange->base);
    putlit(" - ");
    putaddr64(arange->base + arange->size);
    putlit(" | ");
    puthex2(arange->size, 0);
    putlit(" (");
    putint(arange->size, 0);
    putlit(" bytes)");
    put('\n');

    return DW_CB_OK;
}
static enum dw_cb_status aranges_cb(struct dwarf *dwarf, dwarf_aranges_t *aranges)
{
    const size_t maxn = STRLEN(
        "  Length:                   2147483647\n"
        "  Version:                  2147483647\n"
        "  Offset:                   0xffffffffffffffff\n"
        "  Pointer Size:             0xffffffffffffffff\n"
        "  Segment Size:             0xffffffffffffffff\n"
        "\n"
        "  Seg From                 To                   Length\n"
    );
    if (!checkquota(maxn)) {
        flushoutput(buffer, buffersz);
        /* No need to check again (maxn is constant) */
    }
    putlit("  Length:                   ");
    putint(aranges->length, 0);
    put('\n');

    putlit("  Version:                  ");
    putint(aranges->version, 0);
    put('\n');

    putlit("  Offset:                   ");
    putaddr(aranges->debug_info_offset);
    put('\n');

    putlit("  Pointer Size:             ");
    putaddr(aranges->address_size);
    put('\n');

    putlit("  Segment Size:             ");
    putaddr(aranges->segment_size);
    put('\n');

    put('\n');
    putlit("  Seg From                 To                   Length\n");

    aranges->arange_cb = arange_cb;

    return DW_CB_OK;
}
static enum dw_cb_status attr_cb(struct dwarf *dwarf, dwarf_die_t *die, dwarf_attr_t *attr)
{
    const size_t maxn
     = die->depth * 2
     + STRLEN(" [] = ")
     + MAX(STRLEN("0xffffffffffffffff"), DWARF_MAX_SYMBOL_NAME)
     + STRLEN("0xffffffffffffffff") /* Assume worst case */
     + STRLEN(" # FORM: ")
     + MAX(STRLEN("0xffffffffffffffff"), DWARF_MAX_SYMBOL_NAME)
     + STRLEN("\n");
    ensurequota(maxn); /* NOTE: See comment in die_cb */
    const char *attr_name = dwarf_get_symbol_name(DW_AT, attr->name);
    const char *form_name = dwarf_get_symbol_name(DW_FORM, attr->form);
    putindent(die->depth * 2);
    putlit(" [");
    if (attr_name) {
        putcstr(attr_name);
    } else {
        puthex2(attr->name, 0);
    }
    putlit("] = ");
    switch (attr->form) {
    case DW_FORM_flag_present:
        putlit("true (implicit)");
        break;
    case DW_FORM_flag:
        if (attr->value.b) {
            putlit("true");
        } else {
            putlit("false");
        }
        break;
    case DW_FORM_data1:
    case DW_FORM_ref1:
    case DW_FORM_data2:
    case DW_FORM_ref2:
    case DW_FORM_data4:
    case DW_FORM_ref4:
    case DW_FORM_data8:
    case DW_FORM_ref8:
        puthex2(attr->value.val, 0);
        break;
    case DW_FORM_addr:
        putaddr64((uint64_t)attr->value.addr);
        break;
    case DW_FORM_sec_offset:
        puthex2(attr->value.off, 0);
        break;
    case DW_FORM_string:
        ensurequota(strlen(attr->value.cstr) + STRLEN("''"));
        put('\'');
        putcstr(attr->value.cstr);
        put('\'');
        break;
    case DW_FORM_strp:
        {
            const char *str = (const char *)dwarf->str.section.base + attr->value.stroff;
            ensurequota(strlen(str) + STRLEN("'' (offset 0xffffffffffffffff)"));
            put('\'');
            putcstr(str);
            putlit("\' (offset ");
            puthex2(attr->value.stroff, 0);
            put(')');
        }
        break;
    case DW_FORM_exprloc:
        { /* TODO */
            putlit("???");
        }
        break;
    }
    putlit(" # FORM: ");
    if (form_name) {
        putcstr(form_name);
    } else {
        puthex2(attr->form, 0);
    }
    put('\n');
    return DW_CB_OK;
}
static enum dw_cb_status die_cb(struct dwarf *dwarf, dwarf_die_t *die)
{
    const size_t maxn = STRLEN(
        "<0xffffffffffffffff>  [has children]\n"
        "  .abbrev_code:   0xffffffffffffffff\n"
        "  .abbrev_offset: 0xffffffffffffffff\n"
    ) + DWARF_MAX_SYMBOL_NAME
      + die->depth * 6 + 4;
    ensurequota(maxn);
    /*
    FIXME: die->depth would have to be unreasonably high for a out of memory situation
    Maybe check die->depth somewhere else in the code to ensure it doesn't get too big
    */
    dwarf_abbrev_t *abbrev = dwarf_abbrev_table_find_abbrev_from_code(dwarf, die->abbrev_table, die->abbrev_code);
    const char *tag_name = dwarf_get_symbol_name(DW_TAG, die->tag);
    putindent(die->depth * 2);
    put('<');
    puthex2(die->section_offset, 0);
    put('>');
    if (tag_name) {
        put(' ');
        putcstr(tag_name);
        if (die->has_children) {
            putlit(" [has children]");
        } else {
            putlit(" [no children]");
        }
    }
    put('\n');

    putindent(die->depth * 2);
    putlit("  .abbrev_code:   ");
    puthex2(die->abbrev_code, 0);
    put('\n');

    putindent(die->depth * 2);
    putlit("  .abbrev_offset: ");
    puthex2(abbrev->offset, 0);
    put('\n');

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
    dwarf->abbrev_cb = abbrev_cb;
    dwarf->abbrev_attr_cb = abbrev_attr_cb;
    size_t size = 0;
    const uint8_t *data = mapfile(argv[1], &size);
    loadelf(dwarf, data, size, &errinfo);
    if (data == MAP_FAILED) goto fail;
    dwarf_parse(dwarf, &errinfo);
    flushoutput(buffer, buffersz);

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
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)data;
    Elf32_Shdr *strh = (Elf32_Shdr *)(data + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize));
    const char *strs = (const char *)(data + strh->sh_offset);
    printf("num sections: %d\n", ehdr->e_shnum);
    Elf32_Half i;
    for (i=0; i < ehdr->e_shnum; i++) {
        Elf32_Shdr *shdr = (Elf32_Shdr *)(data + ehdr->e_shoff + (i * ehdr->e_shentsize));
        const char *name = &strs[shdr->sh_name];
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
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)(data);
    Elf64_Shdr *strh = (Elf64_Shdr *)(data + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize));
    const char *strs = (const char *)(data + strh->sh_offset);
    printf("num sections: %d\n", ehdr->e_shnum);
    Elf64_Half i;
    for (i=0; i < ehdr->e_shnum; i++) {
        Elf64_Shdr *shdr = (Elf64_Shdr *)(data + ehdr->e_shoff + (i * ehdr->e_shentsize));
        const char *name = &strs[shdr->sh_name];
        printf("0x%016zx:", (size_t)shdr->sh_offset);
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
    Elf_Ehdr *ehdr = (Elf_Ehdr *)data;
    if (ehdr->e_ident[EI_CLASS] == ELFCLASS32) loadelf32(dwarf, data, size, errinfo);
    if (ehdr->e_ident[EI_CLASS] == ELFCLASS64) loadelf64(dwarf, data, size, errinfo);
}
