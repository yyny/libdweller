#ifndef LIBWANDER_CONFIG_H
#define LIBWANDER_CONFIG_H

#include <libwander_config.h>

#if !defined(__STDC__) || __STDC_VERSION__ <= 201112L
# define WANDER_STATIC_ASSERT_AT_INDIR(cond, line, ...) typedef char static_assertion_##line[(cond)?1:-1]
# define WANDER_STATIC_ASSERT_AT(cond, line, ...) WANDER_STATIC_ASSERT_AT_INDIR(cond, line, __VA_ARGS__)
# define WANDER_STATIC_ASSERT(cond, ...) WANDER_STATIC_ASSERT_AT(cond, __LINE__, __VA_ARGS__)
#else
# define WANDER_STATIC_ASSERT(cond, ...) _Static_assert(cond, __VA_ARGS__)
#endif

#ifdef _WIN32
# define WANDER_INTERNAL(T) extern T
# define WANDER_API(T) extern __declspec(dllexport) T
# define WANDER_FUN(T) T
# define WANDER_VAR(T) T
#else
# define WANDER_INTERNAL(T) extern __attribute__((visibility("hidden"))) T
# define WANDER_API(T) extern T
# define WANDER_FUN(T) T
# define WANDER_VAR(T) T
#endif

#endif /* !defined(LIBWANDER_CONFIG_H) */
