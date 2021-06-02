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
#ifndef DWELLER_MACROS_H
#define DWELLER_MACROS_H

#define DW_MAX(a, b) ((a) >= (b) ? (a) : (b))
#define DW_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define DW_STRLEN(str) (sizeof(str) - 1)
#define DW_ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define DW_CONCAT2_INDIR(A, B) A##B
#define DW_CONCAT2(A, B) DW_CONCAT2_INDIR(A, B)
#define DW_CONCAT3_INDIR(A, B, C) A##B##C
#define DW_CONCAT3(A, B, C) DW_CONCAT3_INDIR(A, B, C)

#endif /* DWELLER_MACROS_H */
