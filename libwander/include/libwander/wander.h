#ifndef LIBWANDER_WANDER_H
#define LIBWANDER_WANDER_H

#include <libwander/config.h>

#include <stddef.h>
#include <stdint.h>

typedef size_t wander_thread_id_t;

typedef struct wander_handlers wander_handlers_t;
typedef struct wander_resolver wander_resolver_t;

typedef struct wander_backtrace wander_backtrace_t;
typedef struct wander_frame wander_frame_t;
typedef struct wander_source wander_source_t;
typedef struct wander_symbol wander_symbol_t;
typedef struct wander_resolution wander_resolution_t;

/**
 * @{frames}    The return addresses of each frame.
 * @{offset}    The number of initial frames to be ignored.
 * @{depth}     The total number of frames.
 * @{max_depth} The maximum number of frames that fit in `frames`.
 * @{thread_id} The id of the thread this backtrace originates from.
 * @{free_fn}   Function to call when destroying this backtrace, or NULL.
 */
struct wander_backtrace {
    void             **frames;
    size_t             offset;
    size_t             depth;
    size_t             max_depth;
    wander_thread_id_t thread_id;
    void             (*free_fn)(void *frames);
};
/**
 * @{backtrace}      The backtrace this stack frame is part of, or NULL.
 * @{return_address} The return address of this stack frame.
 * @{frame_index}    The index of this stack frame, 0 means the topmost frame.
 */
struct wander_frame {
    wander_backtrace_t *backtrace;
    void               *return_address;
    size_t              frame_index;
};
/**
 * @{function}  The name of the function, or NULL.
 * @{filename}  The name of the file, or NULL.
 * @{directory} The name of the directory, or NULL.
 * @{lineno}    The line number (1-based), or 0.
 * @{column}    The column number (1-based), or 0.
 */
struct wander_source {
    const char *function;
    const char *filename;
    const char *directory;
    size_t      lineno;
    size_t      column;
};
/**
 * @{name} The name of this symbol, or NULL if not found.
 * @{addr} The base address of this symbol, or NULL if not found.
 * @{size} The size of this symbol, or SIZE_MAX if unknown.
 */
struct wander_symbol {
    const char *name;
    void       *addr;
    size_t      size;
};
/**
 * @{frame}   The stack frame this resolution is based on.
 * @{object}  The object file that the address originates from.
 *            This string is statically allocated.
 * @{object_base} The base object of the object file in memory.
 * @{source}  The source location that the address originates from, see `wander_source_t`.
 * @{symbol}  The symbol that the address is nearest to, see `wander_symbol_t`.
 * @{inlines} Additional source locations that have been inlined by the compiler.
 * @{num_inlines} The number of inlined source locations.
 * @{free_fn} Function to call when destroying this resolution, or NULL.
 * @{free_inlines_fn} Function to call when destroying this resolution, or NULL.
 */
struct wander_resolution {
    wander_frame_t   frame;
    const char      *object;
    size_t           object_base;
    wander_source_t  source;
    wander_symbol_t  symbol;
    wander_source_t *inlines;
    size_t           num_inlines;
    void           (*free_fn)(void *ptr);
    void           (*free_inlines_fn)(void *ptr);
};

WANDER_API(int)                  wander_init(void);
WANDER_API(void)                 wander_fini(void);

WANDER_API(void)                 wander_handle_signal(int signo); /* AS-safe */
WANDER_API(void)                 wander_handle_sigaction(int signo, void /* siginfo_t */ *info, void *ucontext); /* AS-safe */
WANDER_API(wander_handlers_t*)   wander_install_handlers(const int signals[]);
WANDER_API(void)                 wander_uninstall_handlers(wander_handlers_t *handlers);

WANDER_API(int)                  wander_print_backtrace(void); /* AS-safe */

WANDER_API(wander_thread_id_t)   wander_thread_id(void); /* AS-safe */
WANDER_API(size_t)               wander_stackdepth(size_t max_depth); /* AS-safe */

WANDER_API(wander_backtrace_t)   wander_backtrace(size_t max_depth);
WANDER_API(wander_backtrace_t)   wander_backtrace_safe(void *buffer[], size_t max_depth); /* AS-safe */
WANDER_API(wander_backtrace_t*)  wander_backtrace_rebase(wander_backtrace_t *backtrace, void *base); /* AS-safe */
WANDER_API(wander_backtrace_t*)  wander_backtrace_skip(wander_backtrace_t *backtrace, size_t n); /* AS-safe */
WANDER_API(void)                 wander_backtrace_free(wander_backtrace_t *backtrace); /* AS-safe (if `backtrace->free_fn` is AS-safe) */

WANDER_API(size_t)               wander_backtrace_depth(wander_backtrace_t *backtrace); /* AS-safe */
WANDER_API(wander_thread_id_t)   wander_backtrace_thread_id(wander_backtrace_t *backtrace); /* AS-safe */
WANDER_API(wander_frame_t)       wander_backtrace_frame(wander_backtrace_t *backtrace, size_t frame_idx); /* AS-safe */

WANDER_API(wander_resolver_t*)   wander_resolver_create(size_t max_depth, size_t max_locations);
WANDER_API(int)                  wander_resolver_load(wander_resolver_t *resolver, wander_backtrace_t *backtrace); /* AS-safe */
WANDER_API(void)                 wander_resolver_free(wander_resolver_t **resolver);

WANDER_API(wander_resolution_t*) wander_resolve_addr(wander_resolver_t *resolver, uintptr_t addr);
WANDER_API(wander_resolution_t*) wander_resolve_addr_safe(wander_resolver_t *resolver, uintptr_t addr, wander_resolution_t *resolution); /* AS-safe */

WANDER_API(wander_resolution_t*) wander_resolve_frame(wander_resolver_t *resolver, wander_frame_t frame);
WANDER_API(wander_resolution_t*) wander_resolve_frame_safe(wander_resolver_t *resolver, wander_frame_t frame, wander_resolution_t *resolution); /* AS-safe */
WANDER_API(void)                 wander_destroy_resolution(wander_resolution_t **resolution); /* AS-safe (If `resolution->free_fn` is AS-safe) */

#ifndef WANDER_NO_MACROS
# define wander_frame(resolution)       ((resolution)->frame) /* AS-safe */
# define wander_object(resolution)      ((resolution)->object) /* AS-safe */
# define wander_source(resolution)      ((resolution)->source) /* AS-safe */
# define wander_symbol(resolution)      ((resolution)->symbol) /* AS-safe */
# define wander_inlines(resolution)     ((resolution)->inlines) /* AS-safe */
# define wander_num_inlines(resolution) ((resolution)->num_inlines) /* AS-safe */

# define wander_function(source)  ((source).function) /* AS-safe */
# define wander_filename(source)  ((source).filename) /* AS-safe */
# define wander_directory(source) ((source).directory) /* AS-safe */
# define wander_lineno(source)    ((source).lineno) /* AS-safe */
# define wander_column(source)    ((source).column) /* AS-safe */

# define wander_symname(symbol) ((symbol).name) /* AS-safe */
# define wander_symaddr(symbol) ((symbol).addr) /* AS-safe */
# define wander_symsize(symbol) ((symbol).size) /* AS-safe */

/* Optionally allow zero arguments for `wander_backtrace()` */
# define wander_backtrace(max_depth) wander_backtrace(max_depth + 0) /* AS-safe */

# define wander_backtrace_depth(backtrace) ((backtrace)->depth)
# define wander_backtrace_thread_id(backtrace) ((backtrace)->thread_id)
#endif

#endif /* !defined(LIBWANDER_WANDER_H) */
