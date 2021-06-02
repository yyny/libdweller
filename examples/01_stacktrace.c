#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <libwander/wander.h>
#include <libwander/wander_printer.h>

static int inlined_function(void)
{
    int *ptr = (int *)0xdeadbeef;
    *ptr = 42;
    return *ptr;
}
static int recurse(int depth)
{
    if (depth) return recurse(depth - 1);
    return inlined_function();
}

int main(int argc, const char *argv[])
{
    wander_init();
    wander_install_handlers(NULL);
    recurse(5);
    wander_fini();

    return 0;
}
