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
#include <dweller/dwarf.h>
#include <stdarg.h>

#ifdef NDEBUG
#define check(expr) (void)(expr)
#else
#define check(expr) assert(expr)
#endif
#define has_error(ERROR) dwarf_has_error(ERROR)
#define error(ERROR) do { if (errinfo) { *errinfo = ERROR; } return 0; } while (0)

DWSTATIC(struct dwarf_errarg) errval_ptr(void *val) {
    struct dwarf_errarg res;
    res.err_type = DW_ERRARG_TPTR;
    res.err_val.f = val;
    return res;
}
DWSTATIC(struct dwarf_errarg) errval_fun(int (*val)()) {
    struct dwarf_errarg res;
    res.err_type = DW_ERRARG_TFUN;
    res.err_val.f = val;
    return res;
}
DWSTATIC(struct dwarf_errarg) errval_int(int val) {
    struct dwarf_errarg res;
    res.err_type = DW_ERRARG_TINT;
    res.err_val.i = val;
    return res;
}
DWSTATIC(struct dwarf_errarg) errval_big(dw_u64_t val) {
    struct dwarf_errarg res;
    res.err_type = DW_ERRARG_TBIG;
    res.err_val.u = val;
    return res;
}
DWSTATIC(struct dwarf_errarg) errval_str(const char *val) {
    struct dwarf_errarg res;
    res.err_type = DW_ERRARG_TSTR;
    res.err_val.s = val;
    return res;
}
DWSTATIC(struct dwarf_errarg) errval_fmt(const char *val) {
    struct dwarf_errarg res;
    res.err_type = DW_ERRARG_TFMT;
    res.err_val.s = val;
    return res;
}

DWSTATIC(bool) errval_is_truthy(const struct dwarf_errarg *val) {
    return val->err_val.u;
}

/* Format:
 *   I - Integer
 *   Q - Big (64bit) Integer
 *   P - Pointer
 *   F - Function Pointer
 *   S - String
 */
DWSTATIC(struct dwarf_errinfo)
runtime_error(const char *reason, const char *fmt, ...)
{
    struct dwarf_errinfo info;
    info.err_code = DW_ERRRUN;
    info.err_arg[0] = errval_fmt(reason);
    va_list args;
    if (fmt) {
        size_t i;
        va_start(args, fmt);
        for (i=1; i < DWARF_MAX_ERROR_ARGS; i++) {
            if (!*fmt) break;
            switch (*fmt) {
            case 'I':
                info.err_arg[i] = errval_int(va_arg(args, int));
                break;
            case 'Q':
                info.err_arg[i] = errval_big(va_arg(args, dw_u64_t));
                break;
            case 'P':
                info.err_arg[i] = errval_ptr(va_arg(args, void *));
                break;
            case 'F':
                info.err_arg[i] = errval_fun(va_arg(args, int(*)()));
                break;
            case 'S':
                info.err_arg[i] = errval_str(va_arg(args, char *));
                break;
            default:
                break;
            }
        }
        va_end(args);
    }
    return info;
}
DWSTATIC(struct dwarf_errinfo)
argument_error(int argn, const char *argname, const char *funname,
        const char *reason)
{
    struct dwarf_errinfo info;
    info.err_code = DW_ERRARG;
    info.err_arg[0] = errval_int(argn);
    info.err_arg[1] = errval_str(argname);
    info.err_arg[2] = errval_str(funname);
    info.err_arg[3] = errval_str(reason);
    return info;
}
DWSTATIC(struct dwarf_errinfo)
allocator_error(dw_alloc_t *alloc, int nbytes, void *ptr, const char *what)
{
    struct dwarf_errinfo info;
    info.err_arg[0] = errval_fun(*alloc);
    info.err_arg[1] = errval_int(nbytes);
    info.err_arg[2] = errval_ptr(ptr);
    info.err_arg[3] = errval_str(what);
    if (nbytes == 0) {
        info.err_code = DW_ERRALLOC_DEALLOCATE;
    } else if (ptr == NULL) {
        info.err_code = DW_ERRALLOC_ALLOCATE;
    } else {
        info.err_code = DW_ERRALLOC_RESIZE;
    }
    return info;
}
DWSTATIC(struct dwarf_errinfo)
no_allocator_error(const char *what) {
    struct dwarf_errinfo info;
    info.err_code = DW_ERRALLOC_NOALLOC;
    info.err_arg[0] = errval_str(what);
    return info;
}
