#include "wander_platform.h"

#include <stdint.h>

#define WANDER_QUOTE(x) #x
#define WANDER_STRINGIFY(x) WANDER_QUOTE(x)

struct wander_counter {
    size_t depth;
    size_t max_depth;
};

#if WANDER_CONFIG_UNWIND_METHOD_LIBGCC
static _Unwind_Reason_Code libgcc_unwind_count_cb(struct _Unwind_Context *ctx, void *arg)
{
    struct wander_counter *counter = (struct wander_counter *)arg;
    counter->depth++;
    if (counter->max_depth != 0 && counter->depth >= counter->max_depth) {
        return _URC_END_OF_STACK;
    }
    return _URC_NO_REASON;
}
struct wander_libgcc_unwind_trace_cb_args {
    wander_platform_t *platform;
    wander_backtrace_t *backtrace;
};

static _Unwind_Reason_Code libgcc_unwind_trace_cb(struct _Unwind_Context *ctx, void *arg)
{
    struct wander_libgcc_unwind_trace_cb_args *args = (struct wander_libgcc_unwind_trace_cb_args*)arg;
    wander_platform_t *platform = args->platform;
    wander_backtrace_t *backtrace = args->backtrace;
    uintptr_t ip;

    if (backtrace->depth >= backtrace->max_depth)
        return _URC_END_OF_STACK;
    ip = platform->methods.libgcc._Unwind_GetIP(ctx);
    if (ip == 0x0)
        return _URC_END_OF_STACK;
    backtrace->frames[backtrace->depth++] = (void *)ip;
    return _URC_NO_REASON;
}
#endif
#if WANDER_CONFIG_UNWIND_METHOD_NAIVE
static void *volatile naive_getrip(void)
{
label:
    return &&label;
}
#endif

WANDER_FUN(int) wander_platform_init(wander_platform_t *platform)
{
    wander_libhandle_t handle;
#if WANDER_CONFIG_UNWIND_METHOD_DBGHELP && defined(_WIN32)
    {
        handle = wander_dlopen("DBGHELP.DLL");
        if (handle) {
            platform->method = WANDER_UNWIND_METHOD_DBGHELP;
            struct wander_method_dbghelp *method = &platform->methods.dbghelp;
            method->handle = handle;
            *(void **)(&method->CaptureStackBackTrace) = wander_dlsym(handle, "CaptureStackBackTrace");
            if (method->CaptureStackBackTrace)
                goto success;
            wander_dlclose(handle);
        }
        handle = wander_dlopen("NTDLL.DLL");
        if (handle) {
            platform->method = WANDER_UNWIND_METHOD_DBGHELP;
            struct wander_method_dbghelp *method = &platform->methods.dbghelp;
            method->handle = handle;
            *(void **)(&method->CaptureStackBackTrace) = wander_dlsym(handle, "RtlCaptureStackBackTrace");
            if (method->CaptureStackBackTrace)
                goto success;
            wander_dlclose(handle);
        }
    }
#endif
#if WANDER_CONFIG_UNWIND_METHOD_LIBGCC
    handle = wander_dlopen("libgcc_s.so");
    if (!handle)
        handle = wander_dlopen("libgcc_s.so.1");
    if (!handle)
        handle = wander_dlopen_self();
    if (handle) {
        platform->method = WANDER_UNWIND_METHOD_LIBGCC;
        struct wander_method_libgcc *method = &platform->methods.libgcc;
        method->handle = handle;
        *(void **)(&method->_Unwind_Backtrace) = wander_dlsym(handle, "_Unwind_Backtrace");
        *(void **)(&method->_Unwind_GetIP)     = wander_dlsym(handle, "_Unwind_GetIP");
        if (method->_Unwind_Backtrace && method->_Unwind_GetIP)
            goto success;
        wander_dlclose(handle);
    }
#endif
#if WANDER_CONFIG_UNWIND_METHOD_LIBUNWIND
    handle = wander_dlopen("libunwind-generic.so");
    if (!handle)
        handle = wander_dlopen("libunwind-" WANDER_STRINGIFY(UNW_TARGET) ".so");
    if (handle) {
        platform->method = WANDER_UNWIND_METHOD_LIBUNWIND;
        struct wander_method_libunwind *method = &platform->methods.libunwind;
        method->handle = handle;
#if WANDER_UNWIND_GETCONTEXT_IS_A_MACRO == 0
        *(void **)(&method->unw_getcontext) = wander_dlsym(handle, WANDER_STRINGIFY(UNW_ARCH_OBJ(getcontext)));
#endif
        *(void **)(&method->unw_init_local) = wander_dlsym(handle, WANDER_STRINGIFY(UNW_ARCH_OBJ(init_local)));
        *(void **)(&method->unw_step)       = wander_dlsym(handle, WANDER_STRINGIFY(UNW_ARCH_OBJ(step)));
        *(void **)(&method->unw_get_reg)    = wander_dlsym(handle, WANDER_STRINGIFY(UNW_ARCH_OBJ(get_reg)));
#if WANDER_UNWIND_GETCONTEXT_IS_A_MACRO
        if (method->unw_init_local && method->unw_step && method->unw_get_reg)
#else
        if (method->unw_getcontext && method->unw_init_local && method->unw_step && method->unw_get_reg)
#endif
            goto success;
        wander_dlclose(handle);
    }
#endif
#if WANDER_CONFIG_UNWIND_METHOD_GNULIBC
    handle = wander_dlopen("libc.so");
    if (!handle)
        handle = wander_dlopen("libc.so.6");
    if (handle) {
        platform->method = WANDER_UNWIND_METHOD_GNULIBC;
        struct wander_method_gnulibc *method = &platform->methods.gnulibc;
        method->handle = handle;
        *(void **)(&method->backtrace) = wander_dlsym(handle, "backtrace");
        if (method->backtrace) {
            /* `backtrace` may end up calling malloc, but typically only
             * does so on it's first invocation.
             */
            void *dummy;
            method->backtrace(&dummy, 1);
            goto success;
        }
        wander_dlclose(handle);
    }
#endif
#if WANDER_CONFIG_UNWIND_METHOD_NAIVE
    {
        struct wander_method_naive *method = &platform->methods.naive;
        platform->method = WANDER_UNWIND_METHOD_NAIVE;
    }
    goto success;
#endif
    platform->method = WANDER_UNWIND_METHOD_NONE;
    platform->sym_restore = NULL;
    platform->sym_restore_rt = NULL;
    return -1;

success:
    return 0;
}
WANDER_FUN(void) wander_platform_fini(wander_platform_t *platform)
{
    switch (platform->method) {
#if WANDER_CONFIG_UNWIND_METHOD_LIBGCC
    case WANDER_UNWIND_METHOD_LIBGCC:
        if (platform->methods.libgcc.handle)
            wander_dlclose(platform->methods.libgcc.handle);
        platform->methods.libgcc.handle = NULL;
        break;
#endif
#if WANDER_CONFIG_UNWIND_METHOD_LIBUNWIND
    case WANDER_UNWIND_METHOD_LIBUNWIND:
        if (platform->methods.libunwind.handle)
            wander_dlclose(platform->methods.libunwind.handle);
        platform->methods.libunwind.handle = NULL;
        break;
#endif
#if WANDER_CONFIG_UNWIND_METHOD_GNULIBC
    case WANDER_UNWIND_METHOD_GNULIBC:
        if (platform->methods.gnulibc.handle)
            wander_dlclose(platform->methods.gnulibc.handle);
        platform->methods.gnulibc.handle = NULL;
        break;
#endif
    default:
        break;
    }
    platform->method = WANDER_UNWIND_METHOD_NONE;
}

WANDER_FUN(wander_thread_id_t) wander_platform_thread_id(wander_platform_t *platform)
{
    wander_thread_id_t thread_id = 0;
#if defined(_WIN32)
    thread_id = GetCurrentThreadId();
#elif WANDER_CONFIG_HAVE_PTHREAD_GETTHREADID_NP
    thread_id = pthread_getthreadid_np();
#elif defined(__linux__)
    thread_id = syscall(__NR_gettid);
#elif defined(__sun)
    thread_id = pthread_self();
#elif defined(__APPLE__)
    thread_id = mach_thread_self();
    mach_port_deallocate(mach_task_self(), thread_id);
#elif defined(__NetBSD__)
    thread_id = _lwp_self();
#elif defined(__FreeBSD__)
    long lwpid;
    thr_self(&lwpid);
    thread_id = lwpid;
#elif defined(__DragonFly__)
    thread_id = lwp_gettid();
#else
    WANDER_STATIC_ASSERT(sizeof(pthread_t) <= sizeof(wander_thread_id_t));
    /* Best effort; Pray that it compiles */
    pthread_t current_thread = pthread_self();
    thread_id = current_thread;
#endif
    return thread_id;
}

/**
 * Walk the stack to determine it's depth, using platform specific methods.
 * If `max_depth` is positive, walk at most that many frames.
 * This function is AS-safe.
 */
WANDER_FUN(size_t) wander_platform_stackdepth(wander_platform_t *platform, size_t max_depth)
{
    struct wander_counter counter;
    counter.depth = 0;
    counter.max_depth = max_depth;
    switch (platform->method) {
    case WANDER_UNWIND_METHOD_LIBGCC:
#if WANDER_CONFIG_UNWIND_METHOD_LIBGCC
        {
            struct wander_method_libgcc *method = &platform->methods.libgcc;
            method->_Unwind_Backtrace(libgcc_unwind_count_cb, &counter);
        }
#endif
        break;
    case WANDER_UNWIND_METHOD_LIBUNWIND:
#if WANDER_CONFIG_UNWIND_METHOD_LIBUNWIND
        {
            struct wander_method_libunwind *method = &platform->methods.libunwind;
            unw_cursor_t cursor;
            unw_context_t context;

#if WANDER_UNWIND_GETCONTEXT_IS_A_MACRO
            unw_getcontext(&context);
#else
            (method->unw_getcontext)(&context);
#endif
            method->unw_init_local(&cursor, &context);

            while (method->unw_step(&cursor)) {
                counter.depth++;
                if (counter.max_depth != 0 && counter.depth >= counter.max_depth) {
                    break;
                }
            }
        }
#endif
        break;
    case WANDER_UNWIND_METHOD_NAIVE:
#if WANDER_CONFIG_UNWIND_METHOD_NAIVE
        {
            struct wander_method_naive *method = &platform->methods.naive;
            void **current_frame = __builtin_frame_address(0);
            while (current_frame) {
                current_frame = *current_frame;
                if (current_frame == method->root_frame) break;
                counter.depth++;
                if (counter.max_depth != 0 && counter.depth >= counter.max_depth) {
                    break;
                }
            }
        }
#endif
        break;
    case WANDER_UNWIND_METHOD_DBGHELP:
        /* TODO: It's probably possible to do this */
        break;
    case WANDER_UNWIND_METHOD_GNULIBC:
        break;
    default:
        break;
    }
    if (counter.depth == 0) return WANDER_CONFIG_MAX_STACK_DEPTH;
    return counter.depth;
}
/**
 * Walk the stack and generate a backtrace, using platform specific methods.
 * This function is AS-safe.
 */
WANDER_FUN(wander_backtrace_t) wander_platform_backtrace(wander_platform_t *platform, wander_backtrace_t backtrace)
{
    switch (platform->method) {
    case WANDER_UNWIND_METHOD_DBGHELP:
#if WANDER_CONFIG_UNWIND_METHOD_DBGHELP
        {
            struct wander_method_dbghelp *method = &platform->methods.dbghelp;
            backtrace.depth = method->CaptureStackBackTrace(0, backtrace.max_depth, backtrace.frames, NULL);
        }
#endif
        break;
    case WANDER_UNWIND_METHOD_LIBGCC:
#if WANDER_CONFIG_UNWIND_METHOD_LIBGCC
        {
            struct wander_method_libgcc *method = &platform->methods.libgcc;
            struct wander_libgcc_unwind_trace_cb_args args = {
                platform,
                &backtrace
            };
            method->_Unwind_Backtrace(libgcc_unwind_trace_cb, &args);
        }
#endif
        break;
    case WANDER_UNWIND_METHOD_LIBUNWIND:
#if WANDER_CONFIG_UNWIND_METHOD_LIBUNWIND
        {
            struct wander_method_libunwind *method = &platform->methods.libunwind;
            unw_cursor_t cursor;
            unw_context_t context;

#if WANDER_UNWIND_GETCONTEXT_IS_A_MACRO
            unw_getcontext(&context);
#else
            (method->unw_getcontext)(&context);
#endif
            method->unw_init_local(&cursor, &context);

            while (method->unw_step(&cursor)) {
                unw_word_t ip;

                if (backtrace.depth >= backtrace.max_depth)
                    break;
                method->unw_get_reg(&cursor, UNW_REG_IP, &ip);
                backtrace.frames[backtrace.depth++] = (void *)ip;
            }
        }
#endif
        break;
    case WANDER_UNWIND_METHOD_GNULIBC:
#if WANDER_CONFIG_UNWIND_METHOD_GNULIBC
        {
            struct wander_method_gnulibc *method = &platform->methods.gnulibc;
            backtrace.depth = method->backtrace(backtrace.frames, backtrace.max_depth);
        }
#endif
        break;
    case WANDER_UNWIND_METHOD_NAIVE:
#if WANDER_CONFIG_UNWIND_METHOD_NAIVE
        {
            struct wander_method_naive *method = &platform->methods.naive;
            void *rip = naive_getrip();
            if (backtrace.depth >= backtrace.max_depth)
                break;
            backtrace.frames[backtrace.depth++] = rip;
            void **current_frame = __builtin_frame_address(0);
            while (current_frame) {
                if (backtrace.depth >= backtrace.max_depth)
                    break;
                void *return_address = *(current_frame + 1);
                backtrace.frames[backtrace.depth++] = __builtin_extract_return_addr(return_address);
                current_frame = *current_frame;
                if (current_frame == method->root_frame) break;
            }
        }
#endif
        break;
    default:
        break;
    }
    backtrace.thread_id = wander_platform_thread_id(platform);
    return backtrace;
}
