#ifndef LIBWANDER_WANDER_PRINTER_H
#define LIBWANDER_WANDER_PRINTER_H

#include <libwander/wander.h>

typedef int (*WANDER_SELF)();
typedef int (*wander_writer_t)(WANDER_SELF *self, const void *data, size_t size);
typedef struct wander_snippet wander_snippet_t;
typedef struct wander_printer wander_printer_t;

enum {
    WANDER_COLOR_NEVER,
    WANDER_COLOR_ALWAYS,
    WANDER_COLOR_AUTOMATIC,
    WANDER_COLOR_TERMINAL,
};
#define WANDER_DETAIL_FRAME   0x0001 /* Display frame index */
#define WANDER_DETAIL_ADDRESS 0x0002 /* Display return address */
#define WANDER_DETAIL_SOURCE  0x0004 /* Display source position */
#define WANDER_DETAIL_OBJECT  0x0008 /* Display object file unconditionally */
#define WANDER_DETAIL_SYMBOL  0x0010 /* Display symbol name and offset unconditionally */
#define WANDER_DETAIL_SNIPPET 0x0020 /* Display code snippet */
#define WANDER_DETAIL_SAFE    0x4000 /* `writer` is AS-safe */
#define WANDER_DETAIL_TTY     0x8000 /* `writer` is a terminal */
struct wander_printer {
    wander_writer_t  writer;
    int              snippet_context; /* Number of extra snippet lines to print before and after */
    int              color;
    unsigned         details;
    void            *userdata;
};
struct wander_snippet {
    size_t       line_base;
    size_t       num_lines;
    const char **lines;
    void       (*free_fn)(const char **self);
};

WANDER_API(wander_printer_t) wander_default_printer;
WANDER_API(wander_printer_t) wander_default_safe_printer;

WANDER_API(int) wander_write(wander_writer_t *writer, const char *data, size_t size); /* AS-safe */
WANDER_API(int) wander_writestr(wander_writer_t *writer, const char *str); /* AS-safe */
WANDER_API(int) wander_writedec(wander_writer_t *writer, size_t value); /* AS-safe */
WANDER_API(int) wander_writehex(wander_writer_t *writer, size_t value); /* AS-safe */
WANDER_API(int) wander_writeptr(wander_writer_t *writer, void *value); /* AS-safe */

WANDER_API(int) wander_printer_write(wander_printer_t *printer, const void *data, size_t size); /* AS-safe */
WANDER_API(int) wander_printer_writestr(wander_printer_t *printer, const char *str); /* AS-safe */
WANDER_API(int) wander_printer_writedec(wander_printer_t *printer, size_t value); /* AS-safe */
WANDER_API(int) wander_printer_writehex(wander_printer_t *printer, size_t value); /* AS-safe */
WANDER_API(int) wander_printer_writeptr(wander_printer_t *printer, void *value); /* AS-safe */

WANDER_API(wander_printer_t) wander_printer(wander_writer_t writer, int istty); /* AS-safe */
WANDER_API(wander_printer_t) wander_safe_printer(wander_writer_t writer, int istty); /* AS-safe */
WANDER_API(int) wander_print(wander_printer_t *printer, wander_backtrace_t *backtrace);
WANDER_API(int) wander_print_safe(wander_printer_t *printer, wander_backtrace_t *backtrace); /* AS-safe */

WANDER_API(int)              wander_print_snippet(wander_printer_t *printer, const char *directory, const char *filename, size_t from, size_t to); /* AS-safe */
WANDER_API(wander_snippet_t) wander_get_snippet(wander_printer_t *printer, const char *directory, const char *filename, size_t from, size_t to);
WANDER_API(void)             wander_free_snippet(wander_snippet_t *snippet);

#endif /* !defined(LIBWANDER_WANDER_PRINTER_H) */
