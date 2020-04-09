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
#ifndef DWELLER_ERROR_H
#define DWELLER_ERROR_H
#include <dweller/macros.h>

/* %N writes the argument with that number
 * %[N?...] only writes the content in brackets if argument N is truthy.
 * %[N!...] only writes the content in brackets if argument N is falsy.
 */
#define DWARF_ERRORS(ERROR) \
    ERROR(ERRARG, "bad argument #%1 ('%2') to '%3'%[4? (%4)]") \
    ERROR(ERRRUN, "runtime error%[1?: %1]") \
    ERROR(ERRALLOC_NOALLOC, "no allocator found%[1? (%1)]") \
    ERROR(ERRALLOC_ALLOCATE, "allocator %1 failed to allocate %2 bytes%[4? (%4)]") \
    ERROR(ERRALLOC_DEALLOCATE, "allocator %1 failed to deallocate %2 bytes%[4? (%4)]") \
    ERROR(ERRALLOC_RESIZE, "allocator %1 failed to resize memory block %3 to %2 bytes%[4? (%4)]")
enum dwarf_err {
    DW_OK,
#define DWARF_ERROR(NAME, FMT) DW_CONCAT2(DW_, NAME),
    DWARF_ERRORS(DWARF_ERROR)
#undef DWARF_ERROR
};
#define DWARF_MAX_ERROR_ARGS 4
enum dwarf_err_argtype {
    DW_ERRARG_TPTR,
    DW_ERRARG_TINT,
    DW_ERRARG_TBIG,
    DW_ERRARG_TSTR,
    DW_ERRARG_TFMT,
    DW_ERRARG_TFUN,
};
union dwarf_err_argval {
    void *p;
    int i;
    dw_u64_t u;
    const char *s;
    int (*f)();
};
struct dwarf_errloc {
    const char *file;
    int line;
    int column;
};
struct dwarf_errarg {
    enum dwarf_err_argtype err_type;
    union dwarf_err_argval err_val;
};
#define DWERR_HAVE_LOCATION 0x01
struct dwarf_errinfo {
    enum dwarf_err err_code;
    unsigned err_flags;
    struct dwarf_errloc err_loc;
#ifndef NDEBUG
    struct dwarf_errloc err_dbgloc;
#endif
    struct dwarf_errarg err_arg[DWARF_MAX_ERROR_ARGS];
};

#define DWARF_ERRINFO_INIT { DW_OK }
#define dwarf_has_error(ERROR) (dw_isnull(ERROR) && (ERROR)->err_code != DW_OK)
#endif /* DWELLER_ERROR_H */
