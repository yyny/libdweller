#ifndef LIBWANDER_CONFIG_H
#define LIBWANDER_CONFIG_H

#define WANDER_CONFIG_MAX_STACK_DEPTH 256
#define WANDER_CONFIG_MAX_SOURCE_LOCATIONS 256
#define WANDER_CONFIG_MAX_SHARED_STRING_SIZE 4096

#define WANDER_CONFIG_UNWIND_METHOD_LIBGCC       1
#define WANDER_CONFIG_UNWIND_METHOD_LIBUNWIND    1
#define WANDER_CONFIG_UNWIND_METHOD_LIBBACKTRACE 0 /* TODO */
#define WANDER_CONFIG_UNWIND_METHOD_GNULIBC      1
#define WANDER_CONFIG_UNWIND_METHOD_NAIVE        0

/* Define to 1 if you have the `pthread_getthreadid_np' function. */
#undef WANDER_CONFIG_HAVE_PTHREAD_GETTHREADID_NP

/* Define to 1 if you have the <pthread_np.h> header file. */
#undef WANDER_CONFIG_HAVE_PTHREAD_NP_H

#define WANDER_INTERNAL(T) extern __attribute__((visibility("hidden"))) T
#define WANDER_API(T) extern T
#define WANDER_FUN(T) T
#define WANDER_VAR(T) T

#endif /* !defined(LIBWANDER_CONFIG_H) */
