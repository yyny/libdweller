#define _GNU_SOURCE
#include <link.h>
#include <stdlib.h>
#include <stdio.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <elf.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <dweller/dwarf.h>
#include <dweller/libc.h>

static int debugdir = -1;
static void **root_frame;

static struct function {
    const char *name;
    bool external;
    const char *filename;
    uint64_t decl_file;
    uint64_t decl_line;
    bool set_low_pc;
    void *low_pc;
    bool set_high_pc;
    uint64_t high_pc;
    dw_off_t line_offset;
} fun;

static void *return_addresses[256];
static size_t num_return_addresses;

static struct symbol {
    struct symbol_location *location;
    struct function fun;
} symbols[256];

static struct symbol_location {
    const char *name;
    uint64_t base;
    int fd;
    uint8_t *data;
    size_t size;
    struct dwarf *dwarf;
    struct dwarf_errinfo errinfo;
} symbol_locations[256];
static size_t num_symbol_locations;

static dw_off_t line_offset = -1;

struct symbol_location *current_location = NULL;

static enum dw_cb_status my_die_attr_cb(struct dwarf *dwarf, dwarf_die_t *die, dwarf_attr_t *attr)
{
    if (die->tag == DW_TAG_compile_unit) {
        /* We are just here to search for the offset in `.debug_line` */
        if (attr->name != DW_AT_stmt_list) return DW_CB_OK;
        assert(attr->form == DW_FORM_sec_offset || attr->form == DW_FORM_data4);
        line_offset = attr->value.off;
        return DW_CB_OK;
    }
    if (attr->name == DW_AT_name) {
        if (attr->form == DW_FORM_strp) {
            fun.name = dwarf->str.section.base + attr->value.stroff;
        } else if (attr->form == DW_FORM_string) {
            fun.name = attr->value.cstr;
        }
    }
    if (attr->name == DW_AT_external) fun.external = true;
    if (attr->name == DW_AT_decl_file) fun.decl_file = attr->value.val;
    if (attr->name == DW_AT_decl_line) fun.decl_line = attr->value.val;
    if (attr->name == DW_AT_low_pc) {
        assert(attr->form == DW_FORM_addr);
        fun.low_pc = attr->value.addr;
        fun.set_low_pc = true;
    }
    if (attr->name == DW_AT_high_pc) {
        assert(attr->form == DW_FORM_data8);
        fun.high_pc = attr->value.val;
        fun.set_high_pc = true;
    }
    if (fun.name && fun.external && fun.decl_file && fun.decl_line && fun.set_low_pc && fun.set_high_pc) {
        size_t i;
        for (i=0; i < num_return_addresses; i++) {
            if (symbols[i].location != current_location) continue;
            void *retaddr = return_addresses[i];
            void *retrela = return_addresses[i] - current_location->base;
#if 0
            /* Uncomment this if you want to print the location of _all_ symbols */
            printf("Found a function between %p and %p called '%s' in file %zu line %zu\n", fun.low_pc, fun.low_pc + fun.high_pc, fun.name, fun.decl_file, fun.decl_line);
#endif
            if (fun.low_pc < retrela && retrela < fun.low_pc + fun.high_pc) {
                fun.low_pc += current_location->base;
                symbols[i].fun = fun;
            }
        }
        return DW_CB_DONE;
    }
    return DW_CB_OK;
}
static enum dw_cb_status my_die_cb(struct dwarf *dwarf, dwarf_die_t *die)
{
    if (die->tag == DW_TAG_compile_unit) {
        die->attr_cb = my_die_attr_cb;
        return DW_CB_OK;
    }
    if (die->tag != DW_TAG_subprogram) {
        if (die->depth >= 1) return DW_CB_NEXT;
        return DW_CB_OK;
    }
    memset(&fun, 0x00, sizeof(fun));
    die->attr_cb = my_die_attr_cb;
    return DW_CB_OK;
}
static enum dw_cb_status my_line_cb(struct dwarf *dwarf, struct dwarf_line_program *program)
{
    size_t i;
    for (i=0; i < num_return_addresses; i++) {
        if (symbols[i].location != current_location) continue;
        dw_off_t line_offset = symbols[i].fun.line_offset;
        if (line_offset == program->section_offset) {
            symbols[i].fun.filename = dwarf->line.section.base + program->files[symbols[i].fun.decl_file - 1].name;
        }
    }
    return DW_CB_NEXT;
}

static int
callback(struct dl_phdr_info *info, size_t size, void *ud)
{
    int i, j, k;
    const char *soname = info->dlpi_name;
    if (soname[0] == '\0') soname = "/proc/self/exe";

    for (k = 0; k < info->dlpi_phnum; k++) {
        const ElfW(Phdr) *phdr = &info->dlpi_phdr[k];
        ElfW(Word) p_type = phdr->p_type;
        if (phdr->p_type != PT_LOAD) continue;
        if ((phdr->p_flags & PF_X) == 0) continue;

        for (j=0; j < num_return_addresses; j++) {
            void *retaddr = return_addresses[j];
            if (retaddr >= info->dlpi_addr + phdr->p_vaddr && retaddr < info->dlpi_addr + phdr->p_vaddr + phdr->p_memsz) {
                struct symbol_location *location = NULL;
                printf("Address %p (%zu) is in %s, at offset 0x%zx!\n", retaddr, j, soname, retaddr - (info->dlpi_addr + phdr->p_vaddr));
                printf("This section was mapped at 0x%zx + 0x%zx\n", info->dlpi_addr, phdr->p_vaddr);
                for (i=0; i < num_symbol_locations; i++) {
                    struct symbol_location *locptr = &symbol_locations[i];
                    if (strcmp(soname, locptr->name) == 0) {
                        location = locptr;
                    }
                }
                if (!location) {
                    static struct stat sb;
                    location = &symbol_locations[num_symbol_locations++];
                    location->base = info->dlpi_addr;
                    location->name = soname;
                    if (debugdir != -1) {
                        /* Some libraries are symlinked for backwards
                         * compatability (notably libc)
                         * Try to find the real location
                         */
                        char *sopath = realpath(soname, NULL);
                        size_t sopath_sz = strlen(sopath);
                        /* Create a relative path to search for in debugdir
                         */
                        sopath = realloc(sopath, sopath_sz + 3);
                        if (!sopath) return -1;
                        memmove(sopath + 2, sopath, sopath_sz + 1);
                        sopath[0] = '.';
                        sopath[1] = '/';
                        /* Find it in the debug directory */
                        location->fd = openat(debugdir, sopath, O_RDONLY);
                        free(sopath);
                    }
                    if (location->fd == -1) {
                        /* Let's hope the library itself has debug symbols
                         */
                        location->fd = open(soname, O_RDONLY);
                    }
                    if (location->fd == -1) return -1;
                    if (fstat(location->fd, &sb) == -1) {
                        close(location->fd);
                        return -1;
                    }
                    location->size = sb.st_size;
                    location->data = mmap(NULL, location->size, PROT_READ, MAP_SHARED, location->fd, 0);
                    if (location->data == MAP_FAILED) {
                        close(location->fd);
                        return -1;
                    }
                }
                symbols[j].location = location;
            }
        }
    }

    return 0;
}

int main();

void backtrace_init(void) {
    root_frame = __builtin_frame_address(0);
    root_frame = *root_frame;
    /* The frame of the function that called this one */
}
void backtrace_fini(void) {
    /* TODO */
}

#if CONFIG_USE_LIBGCC
#include <unwind.h>

_Unwind_Reason_Code trace_cb(struct _Unwind_Context *ctx, void *unused)
{
    return_addresses[num_return_addresses++] = _Unwind_GetIP(ctx);
    return _URC_NO_REASON;
}
#elif CONFIG_USE_BACKTRACE
#include <execinfo.h>
#endif

void backtrace_print_stacktrace(void) {
    size_t i;
#if CONFIG_USE_LIBGCC
    /*
    Use libgcc_s, which is required on all LSB linux distributions [1]

    [1] https://refspecs.linuxbase.org/LSB_4.1.0/LSB-Core-generic/LSB-Core-generic/libgcc-s.html
    */
    _Unwind_Backtrace(trace_cb, NULL);
#elif CONFIG_USE_BACKTRACE
    /*
    Or, use libunwind/libc backtrace:
    Note that this uses _Unwind_Backtrace internally, at least on amd64
    */
    num_return_addresses = backtrace(return_addresses, 256);
#else
    /*
    If all else fails, use our own hacky implementation
    */
    void **current_frame = __builtin_frame_address(0);
    for (;;) {
        void *return_address = *(current_frame + 1);
        printf("frame: %p / %p\n", current_frame, return_address);
        return_addresses[num_return_addresses++] = return_address;
        current_frame = *current_frame;
        if (current_frame == root_frame) break;
    }
#endif
    debugdir = open("/usr/lib/debug/", O_RDONLY);
    dl_iterate_phdr(callback, NULL);
    for (i=0; i < num_symbol_locations; i++) {
        struct symbol_location *location = &symbol_locations[i];
        current_location = location;
        dwarf_init(&location->dwarf, &dweller_libc_allocator, &location->errinfo);
        Elf64_Ehdr *ehdr = location->data;
        Elf64_Half i;
        for (i=0; i < ehdr->e_phnum; i++) {
            Elf64_Phdr *phdr = location->data + ehdr->e_phoff + (i * ehdr->e_phentsize);
            Elf64_Word p_flags = phdr->p_flags;
            char prots[4];
            prots[0] = (p_flags & PF_R) ? 'r' : '-';
            prots[1] = (p_flags & PF_W) ? 'w' : '-';
            prots[2] = (p_flags & PF_X) ? 'x' : '-';
            prots[3] = 'p';
            printf("0x%06x: %4s ", phdr->p_offset, prots);
            printf("0x%06x+%zu/0x%06x+%zu", phdr->p_vaddr, phdr->p_filesz, phdr->p_paddr, phdr->p_memsz);
            printf("(0x%02x)", phdr->p_type);
            printf("\n");
        }
        Elf64_Shdr *strh = location->data + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize);
        char *strs = location->data + strh->sh_offset;
        printf("num sections: %d\n", ehdr->e_shnum);
        for (i=0; i < ehdr->e_shnum; i++) {
            Elf64_Shdr *shdr = location->data + ehdr->e_shoff + (i * ehdr->e_shentsize);
            char *name = &strs[shdr->sh_name];
            printf("0x%06x-", shdr->sh_offset);
            printf("0x%06x ( %-6zu )", shdr->sh_offset + shdr->sh_size, shdr->sh_size);
            printf(" [%s (0x%02x)]", name, shdr->sh_type);
            printf("\n");
            struct dwarf_section section;
            section.base = location->data + shdr->sh_offset;
            section.size = shdr->sh_size;
            if      (strcmp(name, ".debug_abbrev") == 0) dwarf_load_section(location->dwarf, DWARF_SECTION_ABBREV, section, &location->errinfo);
            else if (strcmp(name, ".debug_aranges") == 0) dwarf_load_section(location->dwarf, DWARF_SECTION_ARANGES, section, &location->errinfo);
            else if (strcmp(name, ".debug_info") == 0) dwarf_load_section(location->dwarf, DWARF_SECTION_INFO, section, &location->errinfo);
            else if (strcmp(name, ".debug_line") == 0) dwarf_load_section(location->dwarf, DWARF_SECTION_LINE, section, &location->errinfo);
            else if (strcmp(name, ".debug_str") == 0) dwarf_load_section(location->dwarf, DWARF_SECTION_STR, section, &location->errinfo);
        }
        location->dwarf->line_cb = my_line_cb;
        location->dwarf->die_cb = my_die_cb;
        dwarf_parse_section(location->dwarf, DWARF_SECTION_ABBREV, &location->errinfo);
        dwarf_parse_section(location->dwarf, DWARF_SECTION_INFO, &location->errinfo);
        if (line_offset != -1) {
            dwarf_parse_section(location->dwarf, DWARF_SECTION_LINE, &location->errinfo);
        }
        if (dwarf_has_error(&location->errinfo)) {
            dwarf_write_error(&location->errinfo, &dweller_libc_stderr_writer);
        }
    }
    for (i=0; i < num_return_addresses; i++) {
        struct function fun = symbols[i].fun;
        printf("#%-3zu %p in ", i, return_addresses[i]);
        if (fun.name) {
            printf("%s()", fun.name);
        } else {
            printf("???");
        }
        if (fun.filename && fun.decl_line) {
            printf(" at %s:%zu", fun.filename, fun.decl_line);
        }
        printf("\n");
    }
}

void indirection() {
    backtrace_print_stacktrace();
}

int main(void) {
    printf("main starts at %p\n", main);
    printf("indirection starts at %p\n", indirection);
    printf("backtrace_print_stacktrace starts at %p\n", backtrace_print_stacktrace);
    backtrace_init();
    indirection();
    backtrace_fini();
    return 0;
}

