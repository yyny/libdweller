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

inline const char *dwarf_get_symbol_name(enum dwarf_symbol_namespace ns, dw_symval_t value)
{
#define DW_SYMCASE(NAME, VALUE) case VALUE: return "DW_" DW_PREFIX "_" #NAME;
    switch (ns) {
    case DW_AT:
        switch (value) {
#define DW_PREFIX "AT"
            DW_AT_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
        break;
    case DW_TAG:
        switch (value) {
#define DW_PREFIX "TAG"
            DW_TAG_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
        break;
    case DW_FORM:
        switch (value) {
#define DW_PREFIX "FORM"
            DW_FORM_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
        break;
    case DW_LNE:
        switch (value) {
#define DW_PREFIX "LNE"
            DW_LNE_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
        break;
    case DW_LNS:
        switch (value) {
#define DW_PREFIX "LNS"
            DW_LNS_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
        break;
    case DW_VIS:
        switch (value) {
#define DW_PREFIX "VIS"
            DW_VIS_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
        break;
    }
#undef DW_SYMCASE
    return NULL;
}
inline const char *dwarf_get_symbol_shortname(enum dwarf_symbol_namespace ns, dw_symval_t value)
{
#define DW_SYMCASE(NAME, VALUE) case VALUE: return #NAME;
    switch (ns) {
    case DW_AT:
        switch (value) {
#define DW_PREFIX "AT"
            DW_AT_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
    case DW_TAG:
        switch (value) {
#define DW_PREFIX "TAG"
            DW_TAG_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
    case DW_FORM:
        switch (value) {
#define DW_PREFIX "FORM"
            DW_FORM_SYMBOLS(DW_SYMCASE)
#undef DW_PREFIX
        }
    }
#undef DW_SYMCASE
    return NULL;
}

struct dwarf_line_opcode {
#define DW_LINEOP_BASIC    0x01
#define DW_LINEOP_SPECIAL  0x02
#define DW_LINEOP_EXTENDED 0x03
#define DW_LINEOP_TYPEMASK 0x03
#define DW_LINEOP_ARGS     0x04
    uint8_t flags;
    uint8_t argsize;
    union {
        uint8_t *bytes;
        uint64_t *args;
    } arg;
};
