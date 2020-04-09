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
#include <dweller/symbols.h>

#define DW_SYMCASE_AT(NAME, VALUE)         case VALUE: return DW_SYMPREFIX("DW_AT_")         #NAME;
#define DW_SYMCASE_LANG(NAME, VALUE)       case VALUE: return DW_SYMPREFIX("DW_LANG_")       #NAME;
#define DW_SYMCASE_TAG(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_TAG_")        #NAME;
#define DW_SYMCASE_ORD(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_ORD_")        #NAME;
#define DW_SYMCASE_ACCESS(NAME, VALUE)     case VALUE: return DW_SYMPREFIX("DW_ACCESS_")     #NAME;
#define DW_SYMCASE_ATE(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_ATE_")        #NAME;
#define DW_SYMCASE_CC(NAME, VALUE)         case VALUE: return DW_SYMPREFIX("DW_CC_")         #NAME;
#define DW_SYMCASE_CFA(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_CFA_")        #NAME;
#define DW_SYMCASE_CHILDREN(NAME, VALUE)   case VALUE: return DW_SYMPREFIX("DW_CHILDREN_")   #NAME;
#define DW_SYMCASE_DSC(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_DSC_")        #NAME;
#define DW_SYMCASE_FORM(NAME, VALUE)       case VALUE: return DW_SYMPREFIX("DW_FORM_")       #NAME;
#define DW_SYMCASE_ID(NAME, VALUE)         case VALUE: return DW_SYMPREFIX("DW_ID_")         #NAME;
#define DW_SYMCASE_INL(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_INL_")        #NAME;
#define DW_SYMCASE_LNE(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_LNE_")        #NAME;
#define DW_SYMCASE_LNS(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_LNS_")        #NAME;
#define DW_SYMCASE_MACINFO(NAME, VALUE)    case VALUE: return DW_SYMPREFIX("DW_MACINFO_")    #NAME;
#define DW_SYMCASE_OP(NAME, VALUE)         case VALUE: return DW_SYMPREFIX("DW_OP_")         #NAME;
#define DW_SYMCASE_VIRTUALITY(NAME, VALUE) case VALUE: return DW_SYMPREFIX("DW_VIRTUALITY_") #NAME;
#define DW_SYMCASE_VIS(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_VIS_")        #NAME;
#define DW_SYMCASE_DS(NAME, VALUE)         case VALUE: return DW_SYMPREFIX("DW_DS_")         #NAME;
#define DW_SYMCASE_END(NAME, VALUE)        case VALUE: return DW_SYMPREFIX("DW_END_")        #NAME;
#define DW_SYMCASE_UT(NAME, VALUE)         case VALUE: return DW_SYMPREFIX("DW_UT_")         #NAME;

#define DW_SYMNSCASE(NAME) case DW_##NAME: switch (value) { DW_##NAME##_SYMBOLS(DW_SYMCASE_##NAME) } break

inline const char *dwarf_get_symbol_name(enum dwarf_symbol_namespace ns, dw_symval_t value)
{
#define DW_SYMPREFIX(x) x
    switch (ns) {
    DW_SYMNSCASE(AT);
    DW_SYMNSCASE(LANG);
    DW_SYMNSCASE(TAG);
    DW_SYMNSCASE(ORD);
    DW_SYMNSCASE(ACCESS);
    DW_SYMNSCASE(ATE);
    DW_SYMNSCASE(CC);
    DW_SYMNSCASE(CFA);
    DW_SYMNSCASE(CHILDREN);
    DW_SYMNSCASE(DSC);
    DW_SYMNSCASE(FORM);
    DW_SYMNSCASE(ID);
    DW_SYMNSCASE(INL);
    DW_SYMNSCASE(LNE);
    DW_SYMNSCASE(LNS);
    DW_SYMNSCASE(MACINFO);
    DW_SYMNSCASE(OP);
    DW_SYMNSCASE(VIRTUALITY);
    DW_SYMNSCASE(VIS);
    DW_SYMNSCASE(DS);
    DW_SYMNSCASE(END);
    DW_SYMNSCASE(UT);
    case DW_EH: break;
    }
    return NULL;
#undef DW_SYMPREFIX
}
inline const char *dwarf_get_symbol_shortname(enum dwarf_symbol_namespace ns, dw_symval_t value)
{
#define DW_SYMPREFIX(x)
    switch (ns) {
    DW_SYMNSCASE(AT);
    DW_SYMNSCASE(LANG);
    DW_SYMNSCASE(TAG);
    DW_SYMNSCASE(ORD);
    DW_SYMNSCASE(ACCESS);
    DW_SYMNSCASE(ATE);
    DW_SYMNSCASE(CC);
    DW_SYMNSCASE(CFA);
    DW_SYMNSCASE(CHILDREN);
    DW_SYMNSCASE(DSC);
    DW_SYMNSCASE(FORM);
    DW_SYMNSCASE(ID);
    DW_SYMNSCASE(INL);
    DW_SYMNSCASE(LNE);
    DW_SYMNSCASE(LNS);
    DW_SYMNSCASE(MACINFO);
    DW_SYMNSCASE(OP);
    DW_SYMNSCASE(VIRTUALITY);
    DW_SYMNSCASE(VIS);
    DW_SYMNSCASE(DS);
    DW_SYMNSCASE(END);
    DW_SYMNSCASE(UT);
    case DW_EH: break;
    }
    return NULL;
#undef DW_SYMPREFIX
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
