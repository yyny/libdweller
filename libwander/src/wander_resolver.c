#include <libwander/wander.h>

#include "wander_internal.h"

#include <assert.h>

#include <link.h> /* dl_phdr_iterate */
#include <sys/auxv.h> /* getauxval */
#include <sys/mman.h> /* mmap */
#include <sys/types.h>
#include <sys/stat.h> /* struct stat */
#include <fcntl.h> /* O_* */
#include <unistd.h>

#include <stdlib.h> /* malloc, free */
#include <string.h> /* memset */

#include <dweller/dwarf.h>
#include <dweller/stream.h>
#include <dweller/libc.h>

/* NOTE: A lot of code in this file is non-portable.
 * This file is supposed to be a proof-of-concept more-so than production-ready code.
 * I will probably work on a more polished implementation some time in the future.
 */

struct die_data {
    dw_str_t    name;
    uint64_t    decl_file;
    uint64_t    decl_line;
    uint64_t    low_pc;
    bool        have_low_pc;
    uint64_t    high_pc;
    bool        have_high_pc;
    uint64_t    line_offset;
    bool        have_line_offset;
    bool        external;
};

struct function {
    bool                found; /* Found symbol/symbol location */
    bool                found_location; /* Found instruction location */
    dw_str_t            name;
    bool                external;
    dw_str_t            filename;
    dw_str_t            include_dir;
    uint64_t            decl_file;
    uint64_t            decl_line;
    bool                have_low_pc;
    void               *low_pc;
    bool                have_high_pc;
    uint64_t            high_pc;
    bool                have_info_offset;
    dw_off_t            info_offset;
    bool                have_line_offset;
    dw_off_t            line_offset;
    const char         *symname;
    void               *symaddr;
    size_t              symsize;
    struct die_data     die_data;
};

struct object_file {
    const char             *name;
    uint64_t                base;
    int                     fd;
    uint8_t                *data;
    size_t                  size;
    const ElfW(Phdr)       *phdr;
    struct dwarf           *dwarf;
    struct dwarf_errinfo    errinfo;
};

struct symbol {
    void               *address;
    struct object_file *object_file;
    struct function     fun;
};

struct wander_resolver {
    wander_backtrace_t *backtrace;
    size_t              max_depth;

    size_t              num_stack_frames;
    struct symbol      *symbols;

    size_t              max_locations;
    size_t              num_locations;
    wander_source_t    *source_locations;

    size_t              max_object_files;
    size_t              num_object_files;
    struct object_file *object_files;

    struct object_file *current_object_file;

    int                 debug_dir;

    uintptr_t           start_addr; // the address of _start
    uintptr_t           init_addr;  // the address of _init
    uintptr_t           fini_addr;  // the address of _fini

    char                buffer[WANDER_CONFIG_MAX_SHARED_STRING_SIZE];

    void              (*free_fn)(void *ptr);
};

static inline void printstr(struct dwarf *dwarf, dw_str_t str)
{
    dw_stream_t stream;
    switch (str.section) {
    case DWARF_SECTION_LINE:
        dw_stream_initfrom(&stream, DWARF_SECTION_LINE, dwarf->line.section, dwarf->line.section_provider, str.off);
        break;
    case DWARF_SECTION_STR:
        dw_stream_initfrom(&stream, DWARF_SECTION_STR, dwarf->str.section, dwarf->str.section_provider, str.off);
        break;
    case DWARF_SECTION_LINESTR:
        dw_stream_initfrom(&stream, DWARF_SECTION_LINESTR, dwarf->line_str.section, dwarf->line_str.section_provider, str.off);
        break;
    case DWARF_SECTION_INFO:
        dw_stream_initfrom(&stream, DWARF_SECTION_INFO, dwarf->info.section, dwarf->info.section_provider, str.off);
        break;
    default:
        abort();
    }
    if (str.len == -1) {
        str.len = 0;
        dw_i64_t off = dw_stream_tell(&stream);
        while (dw_stream_get8(&stream)) str.len++;
        dw_stream_seek(&stream, str.off);
        dw_stream_seek(&stream, off);
    }
    while (dw_stream_tell(&stream) < str.off + str.len)
        putchar(dw_stream_get8(&stream));
}
static inline char *copy_dwarf_str(struct dwarf *dwarf, dw_str_t str, char **pptr, size_t *max_n)
{
    dw_stream_t stream;
    switch (str.section) {
    case DWARF_SECTION_LINE:
        dw_stream_initfrom(&stream, DWARF_SECTION_LINE, dwarf->line.section, dwarf->line.section_provider, str.off);
        break;
    case DWARF_SECTION_STR:
        dw_stream_initfrom(&stream, DWARF_SECTION_STR, dwarf->str.section, dwarf->str.section_provider, str.off);
        break;
    case DWARF_SECTION_LINESTR:
        dw_stream_initfrom(&stream, DWARF_SECTION_LINESTR, dwarf->line_str.section, dwarf->line_str.section_provider, str.off);
        break;
    case DWARF_SECTION_INFO:
        dw_stream_initfrom(&stream, DWARF_SECTION_INFO, dwarf->info.section, dwarf->info.section_provider, str.off);
        break;
    default:
        return "";
    }
    if (str.len == -1) {
        str.len = 0;
        dw_i64_t off = dw_stream_tell(&stream);
        while (dw_stream_get8(&stream)) str.len++;
        dw_stream_seek(&stream, str.off);
        dw_stream_seek(&stream, off);
    }
    if (str.len >= *max_n) return NULL;
    char *ptr = *pptr;
    char *result = ptr;
    while (dw_stream_tell(&stream) < str.off + str.len) // TODO: Bounds checking
        *(ptr++) = dw_stream_get8(&stream);
    *(ptr++) = '\0';
    *max_n -= str.len + 1;
    *pptr = ptr;
    return result;
}

static void clear_function(struct function *fun)
{
    fun->name.section = DWARF_SECTION_UNKNOWN;
    fun->name.off = 0;
    fun->name.len = 0;
    fun->filename.section = DWARF_SECTION_UNKNOWN;
    fun->filename.off = 0;
    fun->filename.len = 0;
    fun->include_dir.section = DWARF_SECTION_UNKNOWN;
    fun->include_dir.off = 0;
    fun->include_dir.len = 0;
    fun->decl_file = -1;
    fun->decl_line = -1;
    fun->have_low_pc = false;
    fun->have_high_pc = false;
    fun->low_pc = 0;
    fun->high_pc = 0;
}

static int phdr_iterate_callback(struct dl_phdr_info *info, size_t size, void *ud)
{
    wander_resolver_t *resolver = (wander_resolver_t*)ud;

    // TODO: Previously, this function returned -1 on error.
    // However, this will stop the phdr iteration.
    // Find some other way to signal errors

    const char *soname = info->dlpi_name;
    if (!soname || soname[0] == '\0') soname = (const char *)getauxval(AT_EXECFN);
    if (!soname || soname[0] == '\0') soname = "/proc/self/exe";

    for (size_t k = 0; k < info->dlpi_phnum; k++) {
        const ElfW(Phdr) *phdr = &info->dlpi_phdr[k];
        if (info->dlpi_name[0] == '\0') {
            if (phdr->p_type == PT_DYNAMIC) {
                const ElfW(Dyn) *dyn = (ElfW(Dyn) *)(phdr->p_vaddr + info->dlpi_addr);
                for (; dyn->d_tag != DT_NULL; dyn++) {
                    switch (dyn->d_tag) {
                    case DT_INIT:
                        resolver->init_addr = info->dlpi_addr + dyn->d_un.d_ptr;
                        break;
                    case DT_FINI:
                        resolver->fini_addr = info->dlpi_addr + dyn->d_un.d_ptr;
                        break;
                    }
                }
            }
        }
        if (phdr->p_type != PT_LOAD) continue;
        if ((phdr->p_flags & PF_X) == 0) continue;

        struct object_file *object_file = NULL;
        for (size_t i=0; i < resolver->num_object_files; i++) {
            struct object_file *obj_file = &resolver->object_files[i];
            if (strcmp(soname, obj_file->name) == 0) {
                object_file = obj_file;
            }
        }
        if (object_file == NULL) {
            static struct stat sb;
            if (resolver->num_object_files + 1 > resolver->max_object_files) {
                if (resolver->max_object_files == 0) resolver->max_object_files = 1;
                resolver->max_object_files *= 2;
                resolver->object_files = realloc(resolver->object_files, resolver->max_object_files * sizeof(struct object_file));
            }
            object_file = &resolver->object_files[resolver->num_object_files++];
            memset(object_file, 0x00, sizeof(struct object_file));
            object_file->phdr = phdr;
            object_file->base = info->dlpi_addr;
            object_file->name = soname;
            object_file->fd = -1;
            if (resolver->debug_dir != -1) {
                /* Some libraries are symlinked for backwards
                 * compatability (notably libc)
                 * Try to find the real location
                 */
                char *sopath = realpath(soname, NULL);
                if (sopath != NULL) {
                    size_t sopath_sz = strlen(sopath);
                    /* Create a relative path to search for in debug_dir
                     */
                    sopath = realloc(sopath, sopath_sz + 3);
                    if (!sopath) return 0; // TODO: Signal error
                    memmove(sopath + 2, sopath, sopath_sz + 1);
                    sopath[0] = '.';
                    sopath[1] = '/';
                    /* Find it in the debug directory */
                    object_file->fd = openat(resolver->debug_dir, sopath, O_RDONLY);
                    free(sopath);
                }
            }
            if (object_file->fd == -1) {
                /* Let's hope the library itself has debug symbols
                 */
                object_file->fd = open(soname, O_RDONLY);
            }
            if (object_file->fd == -1) return 0; // TODO: Signal error
            if (fstat(object_file->fd, &sb) == -1) {
                close(object_file->fd);
                return 0; // TODO: Signal error
            }
            object_file->size = sb.st_size;
            object_file->data = mmap(NULL, object_file->size, PROT_READ, MAP_SHARED, object_file->fd, 0);
            if (object_file->data == MAP_FAILED) {
                close(object_file->fd);
                return 0; // TODO: Signal error
            }
            dwarf_init(&object_file->dwarf, &dweller_libc_allocator, &object_file->errinfo); // TODO: No allocation after initialization
            object_file->dwarf->data = resolver;
        }
    }

    return 0;
}

static enum dw_cb_status my_die_attr_cb(struct dwarf *dwarf, dwarf_unit_t *unit, dwarf_die_t *die, dwarf_attr_t *attr)
{
    wander_resolver_t *resolver = dwarf->data;
    struct die_data *data = die->data;

    switch (die->tag) {
    case DW_TAG_subprogram:
    case DW_TAG_inlined_subroutine:
        /* Continue below */
        break;
    case DW_TAG_compile_unit:
    case DW_TAG_partial_unit:
        /* We are just here to search for the offset in `.debug_line` */
        for (size_t i=0; i < resolver->num_stack_frames; i++) {
            struct symbol *sym = &resolver->symbols[i];
            struct function *fun = &sym->fun;
            if (fun->have_info_offset) {
                if (attr->name == DW_AT_stmt_list && unit->die.section_offset == fun->info_offset) {
                    assert(attr->form == DW_FORM_sec_offset || attr->form == DW_FORM_data4);
                    fun->line_offset = attr->value.off;
                    fun->have_line_offset = true;
                }
            }
        }
        return DW_CB_OK;
    default:
        /* Not interested */
        return DW_CB_OK;
    }

    switch (attr->name) {
    case 0:
        /* End of DIE */
        for (size_t i=0; i < resolver->num_stack_frames; i++) {
            struct symbol *sym = &resolver->symbols[i];
            struct function *fun = &sym->fun;
            /* Check if our PC is within this function */
            if (data->have_low_pc && data->have_high_pc && sym->object_file) {
                uint64_t pc = (uint64_t)(sym->address - sym->object_file->base);
                bool good = pc > data->low_pc && pc <= data->high_pc; /* Bounds are OK */
                bool better = !fun->found || (data->low_pc >= (uint64_t)fun->low_pc && data->high_pc <= fun->high_pc); /* Bounds are more specific (e.g. nested function) */
                if (good && better) {
                    fun->name      = data->name;
                    fun->decl_file = data->decl_file;
                    fun->decl_line = data->decl_line;
                    fun->low_pc    = (void *)data->low_pc;
                    fun->high_pc   = data->high_pc;
                    fun->external  = data->external;
                    fun->found     = true;
                }
            }
        }
        break;
    case DW_AT_abstract_origin:
        {
            dw_i64_t off = unit->die.section_offset + attr->value.val;
            // if (!dwarf_parse_die_at(dwarf, unit, &off, dwarf->errinfo)) return DW_CB_ERR;
        }
        break;
    case DW_AT_name:
        if (attr->form == DW_FORM_strp) {
            data->name.section = DWARF_SECTION_STR;
            data->name.off = attr->value.stroff;
            data->name.len = -1;
        } else if (attr->form == DW_FORM_string) {
            data->name = attr->value.str;
            assert(data->name.section != DWARF_SECTION_UNKNOWN);
        }
        break;
    case DW_AT_external:
        data->external = true;
    case DW_AT_decl_file:
        data->decl_file = attr->value.val;
        break;
    case DW_AT_decl_line:
        data->decl_line = attr->value.val;
        break;
    case DW_AT_low_pc:
        assert(attr->form == DW_FORM_addr);
        data->low_pc = attr->value.val;
        data->have_low_pc = true;
        break;
    case DW_AT_high_pc:
        if (attr->form == DW_FORM_data1 || attr->form == DW_FORM_data2 || attr->form == DW_FORM_data4 || attr->form == DW_FORM_data8) {
            assert(data->have_low_pc);
            data->high_pc = data->low_pc + attr->value.val;
        } else {
            assert(attr->form == DW_FORM_addr);
            data->high_pc = attr->value.val;
        }
        data->have_high_pc = true;
        break;
    }

    return DW_CB_OK;
}
static enum dw_cb_status my_die_cb(struct dwarf *dwarf, dwarf_unit_t *unit, dwarf_die_t *die)
{
    wander_resolver_t *resolver = dwarf->data;

    for (size_t i=0; i < resolver->num_stack_frames; i++) {
        struct symbol *sym = &resolver->symbols[i];
        struct function *fun = &sym->fun;
        if (fun->found) continue;
        if (sym->object_file != resolver->current_object_file) continue;
        if (fun->have_info_offset && unit->die.section_offset != fun->info_offset) continue;
        switch (die->tag) {
        case DW_TAG_compile_unit:
        case DW_TAG_subprogram:
        case DW_TAG_inlined_subroutine:
        case DW_TAG_partial_unit:;
            struct die_data *die_data = &resolver->symbols[i].fun.die_data; // TODO: Do we even need to allocate multiple die_data?
            memset(die_data, 0x00, sizeof(struct die_data));
            die_data->decl_file = -1;
            die_data->decl_line = -1;
            die->attr_cb = my_die_attr_cb;
            die->data = die_data;
            break;
        default:
            return DW_CB_NEXT;
        }
    }
    return DW_CB_OK; // FIXME: Shouldn't this be DW_CB_NEXT?
}
static enum dw_cb_status my_cu_cb(struct dwarf *dwarf, dwarf_cu_t *cu)
{
    wander_resolver_t *resolver = dwarf->data;

    for (size_t i=0; i < resolver->num_stack_frames; i++) {
        struct symbol *sym = &resolver->symbols[i];
        struct function *fun = &sym->fun;
        if (fun->found) continue;
        if (sym->object_file != resolver->current_object_file) continue;
        if (fun->have_info_offset && cu->unit.die.section_offset != fun->info_offset) continue;
        cu->unit.die_cb = my_die_cb;
    }
    return cu->unit.die_cb == NULL ? DW_CB_NEXT : DW_CB_OK;
}
static enum dw_cb_status my_line_row_cb(struct dwarf *dwarf, struct dwarf_line_program *program, struct dwarf_line_program_state *state, struct dwarf_line_program_state *last_state)
{
    wander_resolver_t *resolver = dwarf->data;

    for (size_t i=0; i < resolver->num_stack_frames; i++) {
        struct symbol *sym = &resolver->symbols[i];
        struct function *fun = &sym->fun;
        if (fun->found_location) continue;
        if (sym->object_file != resolver->current_object_file) continue;
        if (!fun->have_line_offset || program->section_offset != fun->line_offset) continue;
        uintptr_t addr = (uintptr_t)(sym->address - sym->object_file->base);
        if (addr >= last_state->address && addr <= state->address) {
            /* Since a `call` instruction pushes the address AFTER the call, we must check last_state to get the correct line-number */
            if (last_state->file == 0) continue;
            assert(last_state->file > 0 && last_state->file <= program->num_files);
            struct dwarf_fileinfo *file = &program->files[last_state->file - 1];
            fun->found_location = true;
            fun->decl_file = last_state->file;
            fun->decl_line = last_state->line;
            fun->filename = file->name;
            if (file->include_directory_idx != 0) {
                struct dwarf_pathinfo info = program->include_directories[file->include_directory_idx - 1];
                switch (info.form) {
                case DW_FORM_string:
                    fun->include_dir.section = DWARF_SECTION_LINE;
                    fun->include_dir.off = info.value.str.off;
                    fun->include_dir.len = info.value.str.len;
                    break;
                case DW_FORM_strp:
                    fun->include_dir.section = DWARF_SECTION_STR;
                    fun->include_dir.off = info.value.stroff;
                    fun->include_dir.len = -1;
                    break;
                }
            }
        }
    }
    return DW_CB_OK;
}
static enum dw_cb_status my_line_cb(struct dwarf *dwarf, struct dwarf_line_program *program)
{
    wander_resolver_t *resolver = dwarf->data;

    for (size_t i=0; i < resolver->num_stack_frames; i++) {
        struct symbol *sym = &resolver->symbols[i];
        struct function *fun = &sym->fun;
        if (sym->object_file != resolver->current_object_file) continue;
        if (!fun->have_line_offset || program->section_offset != fun->line_offset) continue;
        if (fun->decl_file != -1) {
            assert(fun->decl_file > 0 && fun->decl_file <= program->num_files);
            struct dwarf_fileinfo *file = &program->files[fun->decl_file - 1];
            if (file->include_directory_idx != 0) {
                struct dwarf_pathinfo info = program->include_directories[file->include_directory_idx - 1];
                assert(info.form == DW_FORM_string);
                fun->include_dir = info.value.str;
                fun->include_dir.section = DWARF_SECTION_LINE;
            }
            fun->filename = file->name;
        }
        program->line_row_cb = my_line_row_cb;
    }
    return DW_CB_OK;
}
static enum dw_cb_status my_arange_cb(struct dwarf *dwarf, dwarf_aranges_t *aranges, dwarf_arange_t *arange)
{
    wander_resolver_t *resolver = dwarf->data;

    for (size_t i=0; i < resolver->num_stack_frames; i++) {
        uintptr_t retaddr = (uintptr_t)resolver->symbols[i].address;
        struct function *fun = &resolver->symbols[i].fun;
        if (retaddr > resolver->current_object_file->base + arange->base && retaddr <= resolver->current_object_file->base + arange->base + arange->size) {
            fun->info_offset = aranges->debug_info_offset;
            fun->have_info_offset = true;
        }
    }
    return DW_CB_OK;
}

/**
 * Create a new symbol resolver capable of resolving symbols for at least `max_depth` frames and `max_locations` locations.
 */
WANDER_FUN(wander_resolver_t*) wander_resolver_create(size_t max_depth, size_t max_locations)
{
    wander_resolver_t *resolver = malloc(sizeof(wander_resolver_t));
    memset(resolver, 0x00, sizeof(wander_resolver_t));
    resolver->max_depth = max_depth;
    resolver->max_locations = max_locations;
    resolver->free_fn = free;
    resolver->backtrace = NULL;
    resolver->debug_dir = open("/usr/lib/debug/", O_RDONLY);
    resolver->symbols = malloc(max_depth * sizeof(struct symbol));
    dl_iterate_phdr(phdr_iterate_callback, resolver);
    if (wander_global.platform.sym_restore == NULL && wander_global.platform.sym_restore_rt == NULL) {
        for (size_t i=0; i < resolver->num_object_files; i++) {
            struct object_file *object_file = &resolver->object_files[i];
            Elf64_Ehdr *ehdr = (Elf64_Ehdr *)object_file->data;
            if (ehdr == NULL) continue; /* object file is not mapped */
            bool is_libc = strstr(object_file->name, "/libc.so"); /* Do a very rough comparison so it works on multi-arch setups too */
            if (!is_libc) continue;
            Elf64_Shdr *shdrs = (Elf64_Shdr *)(object_file->data + ehdr->e_shoff);
            for (size_t j=0; j < ehdr->e_shnum; j++) {
                assert(sizeof(Elf64_Shdr) == ehdr->e_shentsize);
                Elf64_Shdr *shdr = &shdrs[j];
                switch (shdr->sh_type) {
                case SHT_SYMTAB:
                    {
                        Elf64_Shdr *symtab = shdr;
                        Elf64_Shdr *strh = &shdrs[symtab->sh_link];
                        char *strs = (char *)(object_file->data + strh->sh_offset);
                        Elf64_Sym *sym = (Elf64_Sym *)(object_file->data + symtab->sh_offset);
                        size_t size = symtab->sh_size;
                        while (size > sizeof(Elf64_Sym)) {
                            /* FIXME: Are these symbols linux-specific? */
                            if (strcmp(&strs[sym->st_name], "__restore") == 0) {
                                wander_global.platform.sym_restore = (void *)(object_file->base + sym->st_value);
                            }
                            if (strcmp(&strs[sym->st_name], "__restore_rt") == 0) {
                                wander_global.platform.sym_restore_rt = (void *)(object_file->base + sym->st_value);
                            }
                            sym++;
                            size -= sizeof(Elf64_Sym);
                        }
                    }
                    break;
                }
            }
        }
    }
    return resolver;
}
WANDER_FUN(int) wander_resolver_load(wander_resolver_t *resolver, wander_backtrace_t *backtrace)
{
    resolver->backtrace = backtrace;
    resolver->num_stack_frames = backtrace->depth;
    for (size_t i=0; i < resolver->num_stack_frames; i++) {
        struct symbol *sym = &resolver->symbols[i];
        memset(sym, 0x00, sizeof(struct symbol));
        sym->address = backtrace->frames[i];
        struct function *fun = &resolver->symbols[i].fun;
        clear_function(fun);
        for (size_t k = 0; k < resolver->num_object_files; k++) {
            struct object_file *object_file = &resolver->object_files[k];
            void *retaddr = sym->address;
            if (retaddr >= (void *)object_file->base + object_file->phdr->p_vaddr && retaddr < (void *)object_file->base + object_file->phdr->p_vaddr + object_file->phdr->p_memsz) {
                sym->object_file = object_file;
                break;
            }
        }
        if (sym->address == resolver->init_addr) {
            fun->symname = "_init";
            fun->symaddr = sym->address;
            fun->symsize = 0;
        }
        if (sym->address == resolver->fini_addr) {
            fun->symname = "_fini";
            fun->symaddr = sym->address;
            fun->symsize = 0;
        }
        if (sym->address == resolver->start_addr) {
            fun->symname = "_start";
            fun->symaddr = sym->address;
            fun->symsize = 0;
        }
    }
    for (size_t i=0; i < resolver->num_object_files; i++) {
        struct object_file *object_file = &resolver->object_files[i];
        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)object_file->data;
        if (ehdr == NULL) continue; /* object file is not mapped */
        resolver->current_object_file = object_file;
        if (i == 0) {
            resolver->start_addr = object_file->base + ehdr->e_entry;
        }
        Elf64_Shdr *shstrh = (Elf64_Shdr *)(object_file->data + ehdr->e_shoff + (ehdr->e_shstrndx * ehdr->e_shentsize));
        char *shstrs = (char *)(object_file->data + shstrh->sh_offset);
        Elf64_Shdr *shdrs = (Elf64_Shdr *)(object_file->data + ehdr->e_shoff);
        for (size_t j=0; j < ehdr->e_shnum; j++) {
            assert(sizeof(Elf64_Shdr) == ehdr->e_shentsize);
            Elf64_Shdr *shdr = &shdrs[j];
            switch (shdr->sh_type) {
            case SHT_SYMTAB:
                {
                    Elf64_Shdr *symtab = shdr;
                    Elf64_Shdr *strh = &shdrs[symtab->sh_link];
                    char *strs = (char *)(object_file->data + strh->sh_offset);
                    Elf64_Sym *sym = (Elf64_Sym *)(object_file->data + symtab->sh_offset);
                    size_t size = symtab->sh_size;
                    while (size > sizeof(Elf64_Sym)) {
                        for (size_t k=0; k < resolver->num_stack_frames; k++) {
                            struct symbol *funsym = &resolver->symbols[k];
                            struct function *fun = &funsym->fun;
                            if (fun->found && fun->name.section != DWARF_SECTION_UNKNOWN) continue;
                            bool is_same = funsym->address == (void *)(object_file->base + sym->st_value);
                            bool in_range = funsym->address >= (void *)(object_file->base + sym->st_value)
                                         && funsym->address < (void *)(object_file->base + sym->st_value + sym->st_size);
                            if (is_same || in_range) {
                                fun->symname = &strs[sym->st_name];
                                fun->symaddr = (void *)(object_file->base + sym->st_value);
                                fun->symsize = sym->st_size;
                            }
                        }
                        sym++;
                        size -= sizeof(Elf64_Sym);
                    }
                }
                break;
            }
            char *name = &shstrs[shdr->sh_name];
            struct dwarf_section section;
            section.base = object_file->data + shdr->sh_offset;
            section.size = shdr->sh_size;
            if      (strcmp(name, ".debug_abbrev") == 0) dwarf_load_section(object_file->dwarf, DWARF_SECTION_ABBREV, section, &object_file->errinfo);
            else if (strcmp(name, ".debug_aranges") == 0) dwarf_load_section(object_file->dwarf, DWARF_SECTION_ARANGES, section, &object_file->errinfo);
            else if (strcmp(name, ".debug_info") == 0) dwarf_load_section(object_file->dwarf, DWARF_SECTION_INFO, section, &object_file->errinfo);
            else if (strcmp(name, ".debug_line") == 0) dwarf_load_section(object_file->dwarf, DWARF_SECTION_LINE, section, &object_file->errinfo);
            else if (strcmp(name, ".debug_str") == 0) dwarf_load_section(object_file->dwarf, DWARF_SECTION_STR, section, &object_file->errinfo);
        }
        object_file->dwarf->arange_cb = my_arange_cb;
        object_file->dwarf->line_cb = my_line_cb;
        object_file->dwarf->cu_cb = my_cu_cb;
        if (dwarf_has_section(object_file->dwarf, DWARF_SECTION_ABBREV, &object_file->errinfo)) dwarf_parse_section(object_file->dwarf, DWARF_SECTION_ABBREV, &object_file->errinfo);
        if (dwarf_has_section(object_file->dwarf, DWARF_SECTION_ARANGES, &object_file->errinfo)) dwarf_parse_section(object_file->dwarf, DWARF_SECTION_ARANGES, &object_file->errinfo);
        if (dwarf_has_section(object_file->dwarf, DWARF_SECTION_INFO, &object_file->errinfo)) dwarf_parse_section(object_file->dwarf, DWARF_SECTION_INFO, &object_file->errinfo);
        if (dwarf_has_section(object_file->dwarf, DWARF_SECTION_LINE, &object_file->errinfo)) dwarf_parse_section(object_file->dwarf, DWARF_SECTION_LINE, &object_file->errinfo);
        if (dwarf_has_error(&object_file->errinfo)) {
            dwarf_write_error(&object_file->errinfo, &dweller_libc_stderr_writer);
        }
    }
#if 0
    printf("\n-----------------------------------------------------------\n");
    for (size_t i=0; i < resolver->num_stack_frames; i++) {
        struct symbol *sym = &resolver->symbols[i];
        struct function fun = sym->fun;
        struct object_file *object_file = sym->object_file;
        struct dwarf *dwarf = NULL;
        if (object_file != NULL) dwarf = object_file->dwarf;
        printf("#%-3zu 0x%016lx in ", i, resolver->backtrace->frames[i]);
        if (fun.name.section != DWARF_SECTION_UNKNOWN) {
            printstr(dwarf, fun.name);
            printf("()");
        } else if (fun.symname) {
            printf("[%s + 0x%lx]", fun.symname, sym->address - fun.symaddr);
        } else {
            printf("???");
        }
        if (fun.include_dir.len && fun.filename.len && fun.decl_line) {
            printf(" at ");
            if (fun.include_dir.section != DWARF_SECTION_UNKNOWN) {
                printstr(dwarf, fun.include_dir);
                putchar('/');
            }
            if (fun.filename.section != DWARF_SECTION_UNKNOWN) {
                printstr(dwarf, fun.filename);
            }
            printf(":%zu", fun.decl_line);
        }
        printf("\n");
    }
    printf("\n-----------------------------------------------------------\n");
    fflush(stdout);
#endif
    return 0;
}
WANDER_FUN(void) wander_resolver_free(wander_resolver_t **resolver)
{
    if ((*resolver)->free_fn != NULL) {
        (*resolver)->free_fn(*resolver);
    }
    *resolver = NULL;
}

/**
 * Resolve an arbitrary address.
 * Note that this function does not cache any debug information.
 * If you want to resolve multiple addresses from a backtrace, prefer loading the entire backtrace
 * into the resolver with `wander_resolver_load`, and then query individual frames with `wander_resolve_frame_safe`.
 *
 * This function is AS-safe.
 */
WANDER_FUN(wander_resolution_t*) wander_resolve_addr_safe(wander_resolver_t *resolver, uintptr_t addr, wander_resolution_t *resolution)
{
    /* TODO */
    return resolution;
}
/**
 * This function returns a newly allocated resolution that should be destroyed with `wander_destroy_resolution`.
 * For more information, see `wander_resolve_addr_safe`.
 */
WANDER_FUN(wander_resolution_t*) wander_resolve_addr(wander_resolver_t *resolver, uintptr_t addr)
{
    wander_resolution_t *resolution = malloc(sizeof(wander_resolution_t));
    resolution = wander_resolve_addr_safe(resolver, addr, resolution);
    if (resolution != NULL) {
        resolution->free_fn = free;
    }
    return resolution;
}

/**
 * Resolve a address from a stack frame in a backtrace.
 * For increased performance, load the entire backtrace into the resolver with `wander_resolver_load`.
 * If the backtrace is not loaded, this function will fall back to using `wander_resolve_addr_safe`, which is slower.
 *
 * This function is AS-safe.
 */
WANDER_FUN(wander_resolution_t*) wander_resolve_frame_safe(wander_resolver_t *resolver, wander_frame_t frame, wander_resolution_t *resolution)
{
    memset(resolution, 0x00, sizeof(wander_resolution_t));
    resolution->frame = frame;
    if (frame.backtrace != NULL && frame.backtrace == resolver->backtrace) {
        size_t idx = frame.frame_index;
        struct symbol *sym = &resolver->symbols[idx];
        struct function fun = sym->fun;
        struct object_file *object_file = sym->object_file;
        struct dwarf *dwarf = NULL;
        if (object_file != NULL) dwarf = object_file->dwarf;
        char *ptr = resolver->buffer;
        size_t max_n = sizeof(resolver->buffer);
        resolution->object = object_file->name;
        resolution->object_base = object_file->base;
        if (fun.decl_line != -1) {
            resolution->source.lineno = fun.decl_line;
        }
        if (fun.symname) {
            resolution->symbol.name = fun.symname;
            resolution->symbol.addr = fun.symaddr;
            resolution->symbol.size = fun.symsize;
        }
        if (fun.name.section != DWARF_SECTION_UNKNOWN) {
            resolution->source.function = copy_dwarf_str(dwarf, fun.name, &ptr, &max_n);
        }
        if (fun.include_dir.section != DWARF_SECTION_UNKNOWN) {
            resolution->source.directory = copy_dwarf_str(dwarf, fun.include_dir, &ptr, &max_n);
        }
        if (fun.filename.section != DWARF_SECTION_UNKNOWN) {
            resolution->source.filename = copy_dwarf_str(dwarf, fun.filename, &ptr, &max_n);
        }
        return resolution;
    }
    return wander_resolve_addr_safe(resolver, (uintptr_t)frame.return_address, resolution);
}
/**
 * This function returns a newly allocated resolution that should be destroyed with `wander_destroy_resolution`.
 * For more information see `wander_resolve_frame_safe`.
 */
WANDER_FUN(wander_resolution_t*) wander_resolve_frame(wander_resolver_t *resolver, wander_frame_t frame)
{
    wander_resolution_t *resolution = malloc(sizeof(wander_resolution_t));
    resolution = wander_resolve_frame_safe(resolver, frame, resolution);
    if (resolution != NULL) {
        resolution->free_fn = free;
    }
    return resolution;
}
/**
 * Invalidate the resolution and deallocate used memory.
 * This function is AS-safe if `resolution->free_fn` is AS-safe.
 */
WANDER_FUN(void) wander_destroy_resolution(wander_resolution_t **resolution)
{
    if ((*resolution)->free_fn != NULL) {
        (*resolution)->free_fn(*resolution);
    }
    *resolution = NULL;
}
