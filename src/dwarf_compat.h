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
#ifndef alignof
#  define alignof(T) DW_MAXALIGN
#endif

#ifndef inline
#  define inline
#endif

#define cast(T, e) ((T)(e))

#ifdef __GNUC__
static inline int find_first_bit_set(dw_u8_t byte)
{
    return __builtin_ffs(byte);
}
#else
static inline int find_first_bit_set(dw_u8_t byte)
{
    int index = 0;
    if (byte)
        while ((byte & (1 << index++)) == 0);
    return index;
}
#endif
