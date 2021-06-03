#include <libwander/wander.h>

#include "wander_internal.h"

#include <stdint.h>
#include <string.h> /* strlen, memset */
#include <unistd.h> /* write */

static int wander_safe_stderr_writer(WANDER_SELF *self, const void *data, size_t size)
{
    /* wander_printer_t *printer = (wander_printer_t *)((char *)self - offsetof(wander_printer_t, writer)); */
    return write(STDERR_FILENO, data, size);
}
static size_t wander_log10(size_t idx)
{
    size_t res = 1;
    while (idx >= 10) {
        idx /= 10;
        res++;
    }
    return res;
}
static size_t wander_log16(size_t idx)
{
    size_t res = 1;
    while (idx >= 16) {
        idx /= 16;
        res++;
    }
    return res;
}

WANDER_VAR(wander_printer_t) wander_default_printer = {
    .snippet_context = 3,
    .color = WANDER_COLOR_AUTOMATIC,
    .details = WANDER_DETAIL_FRAME | WANDER_DETAIL_ADDRESS | WANDER_DETAIL_SOURCE | WANDER_DETAIL_SNIPPET,
    .writer = wander_safe_stderr_writer
};
WANDER_VAR(wander_printer_t) wander_default_safe_printer = {
    .snippet_context = 3,
    .color = WANDER_COLOR_AUTOMATIC,
    .details = WANDER_DETAIL_FRAME | WANDER_DETAIL_ADDRESS | WANDER_DETAIL_SOURCE | WANDER_DETAIL_SNIPPET | WANDER_DETAIL_SAFE,
    .writer = wander_safe_stderr_writer
};

WANDER_FUN(int) wander_write(wander_writer_t *writer, const char *data, size_t size)
{
    return (*writer)(writer, data, size);
}
WANDER_FUN(int) wander_writestr(wander_writer_t *writer, const char *str)
{
    return wander_write(writer, str, strlen(str));
}
WANDER_FUN(int) wander_writedec(wander_writer_t *writer, size_t value)
{
    char buffer[19 + 1]; /* log10(SIZE_MAX) + '\0' */
    char *ptr = buffer + sizeof(buffer);
    *--ptr = '\0';
    do {
        *--ptr = '0' + (value % 10);
        value /= 10;
    } while (value != 0);
    return wander_writestr(writer, ptr);
}
WANDER_FUN(int) wander_writehex(wander_writer_t *writer, size_t value)
{
    wander_writestr(writer, "0x");
    char buffer[16 + 1];
    char *ptr = buffer + sizeof(buffer);
    *--ptr = '\0';
    do {
        *--ptr = "0123456789abcdef"[value % 16];
        value /= 16;
    } while (value);
    return wander_writestr(writer, ptr);
}
WANDER_FUN(int) wander_writeptr(wander_writer_t *writer, void *value)
{
    uintptr_t addr = (uintptr_t)value;
    char buffer[2 + 16 + 1]; /* strlen("0x") + log16(UINTPTR_MAX) + '\0' */
    char *ptr = buffer + sizeof(buffer);
    size_t size = 16;
    *--ptr = '\0';
    while (size)
        *--ptr = "0123456789abcdef"[(addr >> (4 * (16 - size--))) & 0xf];
    *--ptr = 'x';
    *--ptr = '0';
    return wander_writestr(writer, ptr);
}

/**
 * This function is AS-safe.
 */
WANDER_FUN(int) wander_printer_write(wander_printer_t *printer, const void *data, size_t size)
{
    return wander_write(&printer->writer, data, size);
}
/**
 * This function is AS-safe.
 */
WANDER_FUN(int) wander_printer_writestr(wander_printer_t *printer, const char *str)
{
    return wander_writestr(&printer->writer, str);
}
/**
 * This function is AS-safe.
 */
WANDER_FUN(int) wander_printer_writedec(wander_printer_t *printer, size_t value)
{
    return wander_writedec(&printer->writer, value);
}
/**
 * This function is AS-safe.
 */
WANDER_FUN(int) wander_printer_writehex(wander_printer_t *printer, size_t value)
{
    return wander_writehex(&printer->writer, value);
}
/**
 * This function is AS-safe.
 */
WANDER_FUN(int) wander_printer_writeptr(wander_printer_t *printer, void *value)
{
    return wander_writeptr(&printer->writer, value);
}

/**
 * Create an AS-unsafe stacktrace printer
 * This function is AS-safe.
 */
WANDER_FUN(wander_printer_t) wander_printer(wander_writer_t writer, int istty)
{
    wander_printer_t printer = wander_default_printer;
    if (writer) printer.writer = writer;
    if (istty) printer.details |= WANDER_DETAIL_TTY;
    return printer;
}
/**
 * Create an AS-safe stacktrace printer
 * This function is AS-safe.
 */
WANDER_FUN(wander_printer_t) wander_safe_printer(wander_writer_t writer, int istty)
{
    wander_printer_t printer = wander_default_safe_printer;
    if (writer) printer.writer = writer;
    if (istty) printer.details |= WANDER_DETAIL_TTY;
    return printer;
}
/**
 * Print a stacktrace using the given printer.
 */
WANDER_FUN(int) wander_print(wander_printer_t *printer, wander_backtrace_t *backtrace)
{
    wander_resolver_load(wander_global.resolver, backtrace);
    wander_printer_writestr(printer, "Stack trace (most recent call last):\n");
    size_t max_len = wander_log10(backtrace->depth);
    for (size_t i=backtrace->offset; i < backtrace->depth; i++) {
        size_t idx = backtrace->depth - (i - backtrace->offset) - 1;
        wander_frame_t frame = wander_backtrace_frame(backtrace, idx);
        wander_resolution_t resolution_buffer;
        wander_resolution_t *resolution = wander_resolve_frame_safe(wander_global.resolver, frame, &resolution_buffer);
        wander_printer_writestr(printer, "#");
        wander_printer_writedec(printer, idx);
        size_t len = wander_log10(idx);
        do {
            wander_printer_writestr(printer, " ");
        } while (len++ < max_len);
        wander_printer_writeptr(printer, frame.return_address);
        wander_printer_writestr(printer, " in ");
        if (!resolution) {
            wander_printer_writestr(printer, "???\n");
            continue; /* Not much else we can do at this point */
        }
        wander_source_t source = resolution->source;
        wander_symbol_t symbol = resolution->symbol;
        if (source.function) {
            wander_printer_writestr(printer, source.function);
            wander_printer_writestr(printer, "()");
        } else if (symbol.name) {
            size_t offset = frame.return_address - symbol.addr;
            wander_printer_writestr(printer, "[");
            wander_printer_writestr(printer, symbol.name);
            wander_printer_writestr(printer, " + ");
            wander_printer_writehex(printer, offset);
            wander_printer_writestr(printer, "]");
        } else if (resolution->object) {
            size_t offset = frame.return_address - resolution->object_base;
            wander_printer_writestr(printer, resolution->object);
            wander_printer_writestr(printer, "(+");
            wander_printer_writehex(printer, offset);
            wander_printer_writestr(printer, ")");
        } else {
            wander_printer_writestr(printer, "???");
        }
        if (source.filename && source.lineno) {
            wander_printer_writestr(printer, " at ");
            if (source.directory) {
                wander_printer_writestr(printer, source.directory);
                wander_printer_writestr(printer, "/");
            }
            wander_printer_writestr(printer, source.filename);
            wander_printer_writestr(printer, ":");
            wander_printer_writedec(printer, source.lineno);
            if (source.column) {
                wander_printer_writestr(printer, ":");
                wander_printer_writedec(printer, source.column);
            }
        } else if ((source.function || symbol.name) && resolution->object) {
            wander_printer_writestr(printer, " from ");
            wander_printer_writestr(printer, resolution->object);
        }
        wander_printer_writestr(printer, "\n");
        if (source.directory && source.filename && source.lineno && printer->snippet_context != 0) {
            wander_print_snippet(printer, source.directory, source.filename, source.lineno - printer->snippet_context / 2, source.lineno + printer->snippet_context / 2);
        }
        wander_destroy_resolution(&resolution);
    }

    return 0;
}
/**
 * Print a stacktrace using the given printer.
 * The printer must be AS-safe.
 * This function is AS-safe.
 */
WANDER_FUN(int) wander_print_safe(wander_printer_t *printer, wander_backtrace_t *backtrace)
{
    if ((printer->details & WANDER_DETAIL_SAFE) == 0) return -1;
    return wander_print(printer, backtrace); /* `wander_print` is AS-safe if `printer->writer` is safe */
}

/**
 * This function is AS-safe.
 */
WANDER_FUN(int) wander_print_snippet(wander_printer_t *printer, const char *directory, const char *filename, size_t from, size_t to)
{
    return 0; /* TODO */
}
WANDER_FUN(wander_snippet_t) wander_get_snippet(wander_printer_t *printer, const char *directory, const char *filename, size_t from, size_t to)
{
    wander_snippet_t snippet;
    snippet.line_base = from - printer->snippet_context / 2;
    snippet.num_lines = 0;
    snippet.lines = NULL; /* TODO */
    snippet.free_fn = NULL;
    return snippet;
}
WANDER_FUN(void) wander_free_snippet(wander_snippet_t *snippet)
{
    if (snippet->free_fn)
        snippet->free_fn(snippet->lines);
    snippet->line_base = 0;
    snippet->num_lines = 0;
    snippet->lines = 0;
    snippet->free_fn = NULL;
}
