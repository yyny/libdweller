/****************************************************************************
 *
 * Copyright 2020 The libdweller project contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
#ifndef DWELLER_COMPILER_H
#define DWELLER_COMPILER_H

#ifdef __GNUC__
#  define dw_unlikely(b) __builtin_expect(!!(b),0)
#  define dw_likely(b)   __builtin_expect(!!(b),1)
#  define dw_unreachable __builtin_unreachable()

#  define dw_nonnull_result
#  define dw_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))
#  define dw_mustuse __attribute__((warn_unused_result))
#  define dw_unused __attribute__((unused))

#  define dw_has_include(header) __has_include(header)

/* We would prefer to simply ignore this warning directly inside dw_isnull,
 * but unfortunately, GCC ignores
 *
 *     _Pragma("GCC diagnostic ignored "-Wnonnull-compare")
 *
 * inside a normal expressions. We can work around this with a helper.
 */
#if 0
#  define dw_isnull(x) \
    __extension__ ({ \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Waddress\"") \
        _Pragma("GCC diagnostic ignored \"-Wnonnull-compare\"") \
        bool res_ = (x) == NULL; \
        _Pragma("GCC diagnostic pop") \
        res_; \
    })
#else
static dw_unused bool dw_isnull_helper(void *ptr) { return ptr == NULL; }
#  define dw_isnull(x) dw_isnull_helper(x)
#endif
#  define DW_USE(x) \
    __extension__ ({ \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wunused-result\"") \
        dw_unused __auto_type res_ = (x); \
        _Pragma("GCC diagnostic pop") \
    })
#else
#  define dw_unlikely(b) (b)
#  define dw_likely(b) (b)
#  define dw_unreachable

#  define dw_nonnull_result
#  define dw_nonnull(...)
#  define dw_mustuse
#  define dw_unused

#  define dw_has_include(header) 0
#endif

#define DW_UNUSED(x) ((void)(x))

#ifndef dw_isnull
#  define dw_isnull(x) (x)
#endif
#ifndef DW_USE
#  define DW_USE(x) DW_UNUSED(x)
#endif

#if dw_has_include(<stdint.h>)
#  define DWELLER_HAVE_STDINT_H 1
#endif

#if dw_has_include(<stddef.h>)
#  define DWELLER_HAVE_STDDEF_H 1
#endif

#if dw_has_include(<stdbool.h>)
#  define DWELLER_HAVE_STDBOOL_H 1
#endif

#endif /* DWELLER_COMPILER_H */
