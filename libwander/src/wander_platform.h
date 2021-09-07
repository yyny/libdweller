#ifndef WANDER_PLATFORM_H
#define WANDER_PLATFORM_H

#include <libwander/wander.h>
#include <libwander/wander_printer.h>

/****************************************************************************/
/* #includes for wander_platform_thread_id() */
#if WANDER_CONFIG_HAVE_PTHREAD_GETTHREADID_NP
# include <pthread.h>
# if WANDER_CONFIG_HAVE_PTHREAD_NP_H
#  include <pthread_np.h>
# endif
#elif defined(__linux__)
# include <sys/syscall.h>
# include <unistd.h>
#elif defined(__sun)
# include <pthread.h>
#elif defined(__APPLE__)
# include <mach/mach.h>
#elif defined(__NetBSD__)
# include <lwp.h>
#elif defined(__FreeBSD__)
# include <sys/thr.h>
#elif defined(__DragonFly__)
# include <sys/lwp.h>
#else
# include <pthread.h>
#endif
/****************************************************************************/

/****************************************************************************/
/* #includes for wander_dlopen()/wander_dlsym()/wander_dlclose() */
#if defined(__unix__)
# include <dlfcn.h> /* dlopen */
typedef void *wander_libhandle_t;
#define wander_dlopen(name) dlopen(name, RTLD_LAZY | RTLD_LOCAL)
#define wander_dlopen_self() dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)
#define wander_dlsym(handle, sym) dlsym(handle, sym)
#define wander_dlclose(handle) dlclose(handle)
#elif defined(_WIN32)
# include <winternl.h> /* RtlNtStatusToDosError */
# include <libloaderapi.h> /* LoadLibraryA */
typedef HMODULE wander_libhandle_t;
#define wander_dlopen(name) LoadLibraryA(name)
#define wander_dlopen_self() GetModuleHandleA(NULL)
#define wander_dlsym(handle, sym) GetProcAddress(handle, sym)
#define wander_dlclose(handle) FreeLibrary(handle)
#else
# error "Platform not (yet) supported"
#endif
/****************************************************************************/

#define _XOPEN_SOURCE 700 /* siginfo_t */
#include <signal.h>

#if WANDER_CONFIG_UNWIND_METHOD_LIBGCC
# include <unwind.h>
#endif
#if WANDER_CONFIG_UNWIND_METHOD_LIBUNWIND
# define UNW_LOCAL_ONLY 1
# include <libunwind.h>

/* We could use the build system to check if `unw_tdep_getcontext` is a
 * function-like macro, but this simple check seems sufficient.
 * NOTE: We rely on the fact that `unw_getcontext` does not need to call any
 * other `libunwind` symbols. This is currently the case on ARM and AArch64.
 */
# if defined(__arm__) || defined(__aarch64__)
#  define WANDER_UNWIND_GETCONTEXT_IS_A_MACRO 1
# else
#  define WANDER_UNWIND_GETCONTEXT_IS_A_MACRO 0
# endif
#endif
#if WANDER_CONFIG_UNWIND_METHOD_DBGHELP && defined(_WIN32)
# include <windows.h>
# include <dbghelp.h>
#endif

/* For WANDER_UNWIND_METHOD_NAIVE, the root frame must be initialized by wander_init itself */
#if WANDER_CONFIG_UNWIND_METHOD_NAIVE
# define wander_platform_init_root_frame(platform) (platform)->methods.naive.root_frame = *(void ***)__builtin_frame_address(0);
#else
# define wander_platform_init_root_frame(platform)
#endif

typedef enum wander_method {
    WANDER_UNWIND_METHOD_NONE,
    WANDER_UNWIND_METHOD_DBGHELP,
    WANDER_UNWIND_METHOD_LIBGCC,
    WANDER_UNWIND_METHOD_LIBUNWIND,
    WANDER_UNWIND_METHOD_LIBBACKTRACE,
    WANDER_UNWIND_METHOD_GNULIBC,
    WANDER_UNWIND_METHOD_NAIVE
} wander_method_t;

typedef struct wander_platform wander_platform_t;
struct wander_platform {
    wander_method_t method;
    union {
#if WANDER_CONFIG_UNWIND_METHOD_DBGHELP && defined(_WIN32)
        struct wander_method_dbghelp {
            wander_libhandle_t handle;
            USHORT (*CaptureStackBackTrace WINAPI)(
                _In_      ULONG  FramesToSkip,
                _In_      ULONG  FramesToCapture,
                _Out_     PVOID  *BackTrace,
                _Out_opt_ PULONG BackTraceHash
            );
        } dbghelp;
#endif
#if WANDER_CONFIG_UNWIND_METHOD_LIBGCC
        struct wander_method_libgcc {
            wander_libhandle_t handle;
            _Unwind_Reason_Code (*_Unwind_Backtrace)(_Unwind_Trace_Fn trace, void *trace_arg);
            _Unwind_Word        (*_Unwind_GetIP)(struct _Unwind_Context *ctx);
        } libgcc;
#endif
#if WANDER_CONFIG_UNWIND_METHOD_LIBUNWIND
        struct wander_method_libunwind {
            wander_libhandle_t handle;
#if WANDER_UNWIND_GETCONTEXT_IS_A_MACRO == 0
            int (*unw_getcontext)(unw_context_t *ucp);
#endif
            int (*unw_init_local)(unw_cursor_t *c, unw_context_t *ctxt);
            int (*unw_step)(unw_cursor_t *cp);
            int (*unw_get_reg)(unw_cursor_t *cp, unw_regnum_t reg, unw_word_t *valp);
        } libunwind;
#endif
#if WANDER_CONFIG_UNWIND_METHOD_GNULIBC
        struct wander_method_gnulibc {
            wander_libhandle_t handle;
            int (*backtrace)(void **buffer, int size);
        } gnulibc;
#endif
#if WANDER_CONFIG_UNWIND_METHOD_NAIVE
        struct wander_method_naive {
            void **root_frame;
        } naive;
#endif
    } methods;
    void *sym_restore;
    void *sym_restore_rt;
};

WANDER_INTERNAL(int)  wander_platform_init(wander_platform_t *platform);
WANDER_INTERNAL(void) wander_platform_fini(wander_platform_t *platform);

WANDER_INTERNAL(wander_thread_id_t) wander_platform_thread_id(wander_platform_t *platform); /* AS-safe */
WANDER_INTERNAL(size_t) wander_platform_stackdepth(wander_platform_t *platform, size_t max_depth); /* AS-safe */
WANDER_INTERNAL(wander_backtrace_t) wander_platform_backtrace(wander_platform_t *platform, wander_backtrace_t backtrace); /* AS-safe */

#endif /* !defined(WANDER_PLATFORM_H) */
