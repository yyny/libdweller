/* dl_iterate_phdr example, originally from the GNU man pages
 */
#define _GNU_SOURCE
#include <link.h>
#include <stdlib.h>
#include <stdio.h>

static int
callback(struct dl_phdr_info *info, size_t size, void *data)
{
    int j;

    printf("Name: \"%s\" (%d segments)\n", info->dlpi_name,
            info->dlpi_phnum);

    for (j = 0; j < info->dlpi_phnum; j++) {
        const ElfW(Phdr) *phdr = &info->dlpi_phdr[j];
        ElfW(Word) p_type = phdr->p_type;
        const char *type =
            (p_type == PT_LOAD) ? "PT_LOAD" :
            (p_type == PT_DYNAMIC) ? "PT_DYNAMIC" :
            (p_type == PT_INTERP) ? "PT_INTERP" :
            (p_type == PT_NOTE) ? "PT_NOTE" :
            (p_type == PT_INTERP) ? "PT_INTERP" :
            (p_type == PT_PHDR) ? "PT_PHDR" :
            (p_type == PT_TLS) ? "PT_TLS" :
            (p_type == PT_GNU_EH_FRAME) ? "PT_GNU_EH_FRAME" :
            (p_type == PT_GNU_STACK) ? "PT_GNU_STACK" :
            (p_type == PT_GNU_RELRO) ? "PT_GNU_RELRO" : NULL;
        ElfW(Word) p_flags = phdr->p_flags;
        char prots[4];
        prots[0] = (p_flags & PF_R) ? 'r' : '-';
        prots[1] = (p_flags & PF_W) ? 'w' : '-';
        prots[2] = (p_flags & PF_X) ? 'x' : '-';
        prots[3] = 'p';

        printf("    %2d: %4s [%14p-%14p; memsz:%7lx] flags: 0x%x; ",
                j, prots,
                (void *) (info->dlpi_addr + info->dlpi_phdr[j].p_vaddr),
                (void *) (info->dlpi_addr + info->dlpi_phdr[j].p_vaddr +
                    info->dlpi_phdr[j].p_memsz),
                info->dlpi_phdr[j].p_memsz,
                info->dlpi_phdr[j].p_flags);
        if (type == NULL)
            printf("[other (0x%x)]\n", p_type);
        else
            printf("%s\n", type);
    }

    return 0;
}

int
main(int argc, char *argv[])
{
  printf("address of main: %p\n", main);
  dl_iterate_phdr(callback, NULL);

  exit(EXIT_SUCCESS);
}
