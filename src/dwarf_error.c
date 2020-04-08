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

static bool dwarf_write_error_format(const struct dwarf_errinfo *info, const char **fmt, dw_writer_t *writer, int skip, int depth);
static bool dwarf_write_error_arg(const struct dwarf_errinfo *info, const struct dwarf_errarg *arg, dw_writer_t *writer, int skip, int depth)
{
    char buffer[32];
    int idx = DW_ARRAYSIZE(buffer); /* Write back to front */

    buffer[--idx] = '\0';
    switch (arg->err_type) {
    case DW_ERRARG_TPTR:
    case DW_ERRARG_TFUN:
        {
            uint64_t val = arg->err_val.u;
            int shift;
            for (shift=0; shift < sizeof(val); shift += 4) {
                buffer[--idx] = "0123456789abcdef"[(val >> shift) & 0xf];
            }
            buffer[--idx] = 'x';
            buffer[--idx] = '0';
        }
        break;
    case DW_ERRARG_TINT:
        {
            int val = arg->err_val.i;
            do {
                buffer[--idx] = '0' + (val % 10);
                val /= 10;
            } while (val);
        }
        break;
    case DW_ERRARG_TBIG:
        {
            uint64_t val = arg->err_val.u;
            do {
                buffer[--idx] = '0' + (val % 10);
                val /= 10;
            } while (val);
        }
        break;
    case DW_ERRARG_TSTR:
        return (*writer)(writer, arg->err_val.s, strlen(arg->err_val.s)) >= 0;
    case DW_ERRARG_TFMT:
        {
            const char *fmt = arg->err_val.s;
            return dwarf_write_error_format(info, &fmt, writer, skip, depth);
        }
    }

    return (*writer)(writer, buffer + idx, DW_ARRAYSIZE(buffer) - 1 - idx) >= 0;
}
static const char *dwarf_error_format_strings[] = {
    "success\n",
#define DWARF_ERROR(NAME, FMT) FMT "\n",
    DWARF_ERRORS(DWARF_ERROR)
#undef DWARF_ERROR
};
static bool dwarf_write_error_format(const struct dwarf_errinfo *info, const char **fmt, dw_writer_t *writer, int skip, int depth)
{
    const char *base = *fmt;
    size_t len = 0;
    while (**fmt) {
        if (**fmt == ']' && depth) {
            ++*fmt;
            break;
        }
        if (**fmt == '%') {
            ++*fmt;
            if (**fmt == '%') len++;
            if (len && skip != -1) {
                if ((*writer)(writer, base, len) < 0) return false;
                len = 0;
            }
            switch (**fmt) {
            case '%':
                ++*fmt;
                base = *fmt;
                break;
            case '[':
                {
                    bool write = true;
                    ++*fmt;
                    if (!(**fmt >= '1' && **fmt <= '9')) return false;
                    int argidx = **fmt - '1';
                    ++*fmt;
                    if (**fmt == '!') {
                        write = !errval_is_truthy(&info->err_arg[argidx]);
                    } else { /* Assume ? */
                        write = errval_is_truthy(&info->err_arg[argidx]);
                    }
                    ++*fmt;
                    if (!dwarf_write_error_format(info, fmt, writer, write ? skip : -1, depth + 1)) return false;
                }
                base = *fmt;
                break;
            case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8':
            case '9':
                {
                    int argidx = **fmt - '1';
                    ++*fmt;
                    if (skip != -1) {
                        if (skip + argidx >= DWARF_MAX_ERROR_ARGS) return false;
                        if (!dwarf_write_error_arg(info, &info->err_arg[skip + argidx], writer, skip + argidx + 1, depth)) return false;
                    }
                }
                base = *fmt;
                break;
            default: /* Not recognized, just print it verbatim */
                ++*fmt;
                base = *fmt - 2;
                len = 2;
                break;
            }
        } else {
            len++;
            ++*fmt;
        }
    }

    if (!len) return true;
    if (skip == -1) return true;
    return (*writer)(writer, base, len) >= 0;
}
bool dwarf_write_error(const struct dwarf_errinfo *info, dw_writer_t *writer)
{
    char buffer[] = "Unknown error: 0x%%";
    size_t offset = 0;
    while (buffer[offset] != '%') offset++;
    size_t i;
    if (info->err_code < DW_ARRAYSIZE(dwarf_error_format_strings)) {
        const char *fmt = dwarf_error_format_strings[info->err_code];
        return dwarf_write_error_format(info, &fmt, writer, 0, 0);
    }
    buffer[offset+0] = "0123456789abcdef"[info->err_code & 0xf];
    buffer[offset+1] = "0123456789abcdef"[info->err_code >> 4];
    return (*writer)(writer, buffer, DW_ARRAYSIZE(buffer)) >= 0;
}
