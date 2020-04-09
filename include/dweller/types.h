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
#ifndef DWELLER_TYPES_H
#define DWELLER_TYPES_H

#if defined(_STDINT_H) || defined(STDINT_H) || defined(DWELLER_HAVE_STDINT_H)
#ifdef DWELLER_HAVE_STDINT_H
#include <stdint.h>
#endif
typedef uint8_t dw_u8_t;
typedef uint16_t dw_u16_t;
typedef uint32_t dw_u32_t;
typedef uint64_t dw_u64_t;

typedef int8_t dw_i8_t;
typedef int16_t dw_i16_t;
typedef int32_t dw_i32_t;
typedef int64_t dw_i64_t;

#define DW_MAXALIGN alignof(dw_maxalign_t)
#else
typedef unsigned char dw_u8_t;
typedef unsigned short dw_u16_t;
typedef unsigned int   dw_u32_t;
typedef unsigned long  dw_u64_t;

typedef signed char  dw_i8_t;
typedef signed short dw_i16_t;
typedef signed int   dw_i32_t;
typedef signed long  dw_i64_t;

#define DW_MAXALIGN sizeof(void *)
#endif

#endif /* DWELLER_TYPES_H */
