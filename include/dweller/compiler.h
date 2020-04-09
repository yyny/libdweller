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
#define dw_unlikely(b) __builtin_expect(!!(b),0)
#define dw_likely(b)   __builtin_expect(!!(b),1)
#define dw_unreachable __builtin_unreachable()

#define dw_nonnull_result
#define dw_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))
#define dw_mustuse __attribute__((warn_unused_result))
#define dw_unused __attribute__((unused))

#define dw_isnull(x) \
    ({ \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wnonnull-compare\"") \
        _Pragma("GCC diagnostic ignored \"-Waddress\"") \
        bool res_ = (x); \
        _Pragma("GCC diagnostic pop") \
        res_; \
    })
#define DW_USE(x) \
    ({ \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wunused-result\"") \
        dw_unused __auto_type res_ = (x); \
        _Pragma("GCC diagnostic pop") \
    })
#else
#define dw_unlikely(b) (b)
#define dw_likely(b) (b)
#define dw_unreachable

#define dw_nonnull_result
#define dw_nonnull(...)
#define dw_mustuse
#define dw_unused
#endif

#define DW_UNUSED(x) ((void)(x))

#ifndef dw_isnull
#define dw_isnull(x) (x)
#endif
#ifndef DW_USE
#define DW_USE(x) DW_UNUSED(x)
#endif

#endif /* DWELLER_COMPILER_H */
