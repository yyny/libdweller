#define WANDER_NO_MACROS
#include <libwander/wander.h>
#include <libwander/wander_printer.h>

#include "wander_internal.h"
#include "wander_platform.h"

#include <stdint.h>
#include <stdlib.h> /* malloc, free */
#include <string.h> /* memset */
#include <inttypes.h>

WANDER_VAR(struct wander_global) wander_global;

static void wander_sigaction_handler(int signo, siginfo_t *info, void *ucontext)
{
    wander_handle_sigaction(signo, info, ucontext);
}

WANDER_FUN(int) wander_init(void)
{
    wander_global.resolver = wander_resolver_create(WANDER_CONFIG_MAX_STACK_DEPTH, WANDER_CONFIG_MAX_SOURCE_LOCATIONS);
    wander_platform_init_root_frame(&wander_global.platform);
    return wander_platform_init(&wander_global.platform);
}
WANDER_FUN(void) wander_fini(void)
{
    wander_platform_fini(&wander_global.platform);
    wander_resolver_free(&wander_global.resolver);
    wander_uninstall_handlers(wander_global.signal_handlers);
}

/**
 * This function is AS-safe.
 */
WANDER_FUN(void) wander_handle_signal(int signo)
{
    wander_handle_sigaction(signo, NULL, NULL);
}
/**
 * This function is AS-safe.
 */
WANDER_FUN(void) wander_handle_sigaction(int signo, void *info, void *ucontext)
{
    void *buffer[WANDER_CONFIG_MAX_STACK_DEPTH];
    ucontext_t *uctx = (ucontext_t *)(ucontext);
    void *error_addr = NULL;

    if (uctx) {
#ifdef REG_RIP /* x86_64 */
        error_addr = (void *)(uctx->uc_mcontext.gregs[REG_RIP]);
#elif defined(REG_EIP) /* x86_32 */
        error_addr = (void *)(uctx->uc_mcontext.gregs[REG_EIP]);
#elif defined(__arm__)
        error_addr = (void *)(uctx->uc_mcontext.arm_pc);
#elif defined(__aarch64__)
        error_addr = (void *)(uctx->uc_mcontext.pc);
#elif defined(__mips__)
        error_addr = (void *)((struct sigcontext *)(&uctx->uc_mcontext)->sc_pc);
#elif defined(__ppc__) || defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__)
        error_addr = (void *)(uctx->uc_mcontext.regs->nip);
#elif defined(__s390x__)
        error_addr = (void *)(uctx->uc_mcontext.psw.addr);
#elif defined(__APPLE__) && defined(__x86_64__)
        error_addr = (void *)(uctx->uc_mcontext->__ss.__rip);
#elif defined(__APPLE__)
        error_addr = (void *)(uctx->uc_mcontext->__ss.__eip);
#else
        error_addr = NULL;
#endif
    }
    wander_backtrace_t backtrace = wander_backtrace_safe(buffer, WANDER_CONFIG_MAX_STACK_DEPTH);
    wander_backtrace_rebase(&backtrace, error_addr);
    wander_print(&wander_default_safe_printer, &backtrace);
    if (info) {
#if _XOPEN_SOURCE >= 700 || _POSIX_C_SOURCE >= 200809L
        psiginfo(info, NULL);
#elif defined(REG_ERR)
        siginfo_t *siginfo = info;
        if (signo == SIGSEGV || signo == SIGBUS) {
            error_addr = siginfo->si_addr;
            const char *signal_name = strsignal(signo);
            unsigned error_type = uctx->uc_mcontext.gregs[REG_ERR];
            const char *error_type_str = "access of";
            if (error_type & 0x1) error_type_str = "read from";
            if (error_type & 0x2) error_type_str = "write to";
            wander_writestr(&wander_default_safe_printer.writer, signal_name);
            wander_writestr(&wander_default_safe_printer.writer, " (Invalid ");
            wander_writestr(&wander_default_safe_printer.writer, error_type_str);
            wander_writestr(&wander_default_safe_printer.writer, " address ");
            wander_writeptr(&wander_default_safe_printer.writer, error_addr);
            wander_writestr(&wander_default_safe_printer.writer, ")\n");
        }
#endif
    }
    wander_backtrace_free(&backtrace);
}
/*
 * The first handlers installed with this function will be automatically uninstalled by `wander_fini`.
 * Subsequent handlers have to be uninstalled manually.
 */
WANDER_FUN(wander_handlers_t*) wander_install_handlers(const int signals[])
{
    static const int posix_signals[] = { /* Signals for which the default action is "Core". */
        SIGABRT, /* Abort signal from abort(3)        */
        SIGBUS,  /* Bus error (bad memory access)     */
        SIGFPE,  /* Floating point exception          */
        SIGILL,  /* Illegal Instruction               */
        SIGIOT,  /* IOT trap. A synonym for SIGABRT   */
        SIGQUIT, /* Quit from keyboard                */
        SIGSEGV, /* Invalid memory reference          */
        SIGSYS,  /* Bad argument to routine (SVr4)    */
        SIGTRAP, /* Trace/breakpoint trap             */
        SIGXCPU, /* CPU time limit exceeded (4.2BSD)  */
        SIGXFSZ, /* File size limit exceeded (4.2BSD) */
#if 0
        SIGEMT,  /* Emulation instruction executed    */
#endif
        0,
    };
    size_t num_handlers = 0;
    if (signals == NULL) signals = &posix_signals[0];
    for (; signals[num_handlers]; num_handlers++);
    wander_handlers_t *handlers = malloc(sizeof(wander_handlers_t) + num_handlers * sizeof(wander_handler_t));
    handlers->num_handlers = num_handlers;
    for (size_t i=0; i < num_handlers; i++) {
        int signo = signals[i];
        struct sigaction action;
        handlers->handlers[i].signo = signo;
        memset(&action, 0x00, sizeof(action));
        action.sa_flags = (int)(SA_SIGINFO | SA_ONSTACK | SA_NODEFER | SA_RESETHAND);
        sigfillset(&action.sa_mask);
        sigdelset(&action.sa_mask, signo);
        action.sa_sigaction = &wander_sigaction_handler;

        sigaction(signo, &action, &handlers->handlers[i].old_sigaction);
    }
    if (wander_global.signal_handlers == NULL) {
        wander_global.signal_handlers = handlers;
    }
    return handlers;
}
WANDER_FUN(void) wander_uninstall_handlers(wander_handlers_t *handlers)
{
    for (size_t i=0; i < handlers->num_handlers; i++) {
        sigaction(handlers->handlers[i].signo, &handlers->handlers[i].old_sigaction, NULL);
    }
    if (handlers == wander_global.signal_handlers) {
        wander_global.signal_handlers = NULL;
    }
    free(handlers);
}

WANDER_FUN(int) wander_print_backtrace(void)
{
    static void *buffer[WANDER_CONFIG_MAX_STACK_DEPTH];
    wander_backtrace_t backtrace = wander_backtrace_safe(buffer, WANDER_CONFIG_MAX_STACK_DEPTH);
    return wander_print(&wander_default_safe_printer, &backtrace);
}

/* Tries to give a best effort thread id, which is always an integer.
 * Returns platform provided thread id's whenever possible,
 * see wander_platform.c for implementation.
 */
WANDER_FUN(wander_thread_id_t) wander_thread_id(void)
{
    return wander_platform_thread_id(&wander_global.platform);
}
/**
 * Walk the stack to determine it's depth.
 * If `max_depth` is positive, walk at most that many frames.
 *
 * This function is AS-safe.
 */
WANDER_FUN(size_t) wander_stackdepth(size_t max_depth)
{
    return wander_platform_stackdepth(&wander_global.platform, max_depth);
}

/**
 * Walk the stack and generate a backtrace.
 * Enough space is allocated for at most `max_depth` frames.
 * If `max_depth` is zero, enough space is allocated for every single stack frames.
 */
WANDER_FUN(wander_backtrace_t) wander_backtrace(size_t max_depth)
{
    static wander_backtrace_t null_backtrace;
    wander_backtrace_t backtrace = null_backtrace;
    if (max_depth == 0) {
        max_depth = wander_stackdepth(max_depth);
    }
    if (max_depth == 0) return backtrace;
    backtrace.frames = calloc(sizeof(void *), max_depth);
    if (backtrace.frames != NULL) {
        backtrace.max_depth = max_depth;
        backtrace.free_fn = free;
    }
    return wander_platform_backtrace(&wander_global.platform, backtrace);
}
/**
 * Walk the stack and generate a backtrace.
 * A buffer of at least `max_depth` elements should be pre-allocated by the caller.
 *
 * This function is AS-safe.
 */
WANDER_FUN(wander_backtrace_t) wander_backtrace_safe(void **buffer, size_t max_depth)
{
    static wander_backtrace_t null_backtrace;
    wander_backtrace_t backtrace = null_backtrace;
    if (max_depth == 0) return backtrace;
    backtrace.frames = buffer;
    backtrace.max_depth = max_depth;
    return wander_platform_backtrace(&wander_global.platform, backtrace);
}
/**
 * If there is a stack frame with return address `base`, discard stack frames
 * until it is on top.
 * Pass `NULL` to revert the backtrace to it's original value,
 * before any stackframes were discarded.
 *
 * This function is AS-safe.
 */
WANDER_FUN(wander_backtrace_t*) wander_backtrace_rebase(wander_backtrace_t *backtrace, void *base)
{
    size_t offset;
    if (base == NULL) {
        backtrace->offset = 0;
    } else {
        for (offset = backtrace->offset; offset < backtrace->depth; offset++) {
            if (backtrace->frames[offset] == base) {
                backtrace->offset = offset;
                break;
            }
        }
    }
    return backtrace;
}
/**
 * Discard the topmost `n` stack frames.
 *
 * This function is AS-safe.
 */
WANDER_FUN(wander_backtrace_t*) wander_backtrace_skip(wander_backtrace_t *backtrace, size_t n)
{
    if (n > backtrace->depth || backtrace->offset + n > backtrace->depth) {
        backtrace->offset = backtrace->depth;
    } else {
        backtrace->offset += n;
    }
    return backtrace;
}
/**
 * Invalidate the backtrace and deallocate used memory.
 *
 * This function is AS-safe if `backtrace->free_fn` is AS-safe.
 */
WANDER_FUN(void) wander_backtrace_free(wander_backtrace_t *backtrace)
{
    static wander_backtrace_t null_backtrace;
    if (backtrace->free_fn) {
        backtrace->free_fn(backtrace->frames - backtrace->offset);
        backtrace->free_fn = NULL;
    }
    *backtrace = null_backtrace;
}

/**
 * This function is AS-safe.
 */
WANDER_FUN(size_t) wander_backtrace_depth(wander_backtrace_t *backtrace)
{
    return backtrace->depth;
}
/**
 * This function is AS-safe.
 */
WANDER_FUN(wander_thread_id_t) wander_backtrace_thread_id(wander_backtrace_t *backtrace)
{
    return backtrace->thread_id;
}
/**
 * This function is AS-safe.
 */
WANDER_FUN(wander_frame_t) wander_backtrace_frame(wander_backtrace_t *backtrace, size_t frame_idx)
{
    wander_frame_t frame = { NULL, NULL, 0 };
    if (frame_idx > backtrace->depth) return frame;
    frame.backtrace = backtrace;
    frame.return_address = backtrace->frames[frame_idx];
    frame.frame_index = frame_idx;
    return frame;
}
