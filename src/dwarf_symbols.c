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

#define DW_SYMNAME_AT(NAME, VALUE)         [VALUE] = DW_SYMPREFIX("DW_AT_")         #NAME,
#define DW_SYMNAME_LANG(NAME, VALUE)       [VALUE] = DW_SYMPREFIX("DW_LANG_")       #NAME,
#define DW_SYMNAME_TAG(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_TAG_")        #NAME,
#define DW_SYMNAME_ORD(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_ORD_")        #NAME,
#define DW_SYMNAME_ACCESS(NAME, VALUE)     [VALUE] = DW_SYMPREFIX("DW_ACCESS_")     #NAME,
#define DW_SYMNAME_ATE(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_ATE_")        #NAME,
#define DW_SYMNAME_CC(NAME, VALUE)         [VALUE] = DW_SYMPREFIX("DW_CC_")         #NAME,
#define DW_SYMNAME_CFA(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_CFA_")        #NAME,
#define DW_SYMNAME_CHILDREN(NAME, VALUE)   [VALUE] = DW_SYMPREFIX("DW_CHILDREN_")   #NAME,
#define DW_SYMNAME_DSC(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_DSC_")        #NAME,
#define DW_SYMNAME_FORM(NAME, VALUE)       [VALUE] = DW_SYMPREFIX("DW_FORM_")       #NAME,
#define DW_SYMNAME_ID(NAME, VALUE)         [VALUE] = DW_SYMPREFIX("DW_ID_")         #NAME,
#define DW_SYMNAME_INL(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_INL_")        #NAME,
#define DW_SYMNAME_LNE(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_LNE_")        #NAME,
#define DW_SYMNAME_LNS(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_LNS_")        #NAME,
#define DW_SYMNAME_MACINFO(NAME, VALUE)    [VALUE] = DW_SYMPREFIX("DW_MACINFO_")    #NAME,
#define DW_SYMNAME_OP(NAME, VALUE)         [VALUE] = DW_SYMPREFIX("DW_OP_")         #NAME,
#define DW_SYMNAME_VIRTUALITY(NAME, VALUE) [VALUE] = DW_SYMPREFIX("DW_VIRTUALITY_") #NAME,
#define DW_SYMNAME_VIS(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_VIS_")        #NAME,
#define DW_SYMNAME_DS(NAME, VALUE)         [VALUE] = DW_SYMPREFIX("DW_DS_")         #NAME,
#define DW_SYMNAME_END(NAME, VALUE)        [VALUE] = DW_SYMPREFIX("DW_END_")        #NAME,
#define DW_SYMNAME_UT(NAME, VALUE)         [VALUE] = DW_SYMPREFIX("DW_UT_")         #NAME,

#define DW_SYMPREFIX(x) x
#define DW_SYMNAMES(NAME) const char *dw_##NAME##_names[256] = { DW_##NAME##_SYMBOLS(DW_SYMNAME_##NAME) }
DW_SYMNAMES(AT);
DW_SYMNAMES(LANG);
DW_SYMNAMES(TAG);
DW_SYMNAMES(ORD);
DW_SYMNAMES(ACCESS);
DW_SYMNAMES(ATE);
DW_SYMNAMES(CC);
DW_SYMNAMES(CFA);
DW_SYMNAMES(CHILDREN);
DW_SYMNAMES(DSC);
DW_SYMNAMES(FORM);
DW_SYMNAMES(ID);
DW_SYMNAMES(INL);
DW_SYMNAMES(LNE);
DW_SYMNAMES(LNS);
DW_SYMNAMES(MACINFO);
DW_SYMNAMES(OP);
DW_SYMNAMES(VIRTUALITY);
DW_SYMNAMES(VIS);
DW_SYMNAMES(DS);
DW_SYMNAMES(END);
DW_SYMNAMES(UT);
#undef DW_SYMNAMES
#undef DW_SYMPREFIX

#define DW_SYMPREFIX(x)
#define DW_SYMNAMES(NAME) const char *dw_##NAME##_shortnames[256] = { DW_##NAME##_SYMBOLS(DW_SYMNAME_##NAME) }
DW_SYMNAMES(AT);
DW_SYMNAMES(LANG);
DW_SYMNAMES(TAG);
DW_SYMNAMES(ORD);
DW_SYMNAMES(ACCESS);
DW_SYMNAMES(ATE);
DW_SYMNAMES(CC);
DW_SYMNAMES(CFA);
DW_SYMNAMES(CHILDREN);
DW_SYMNAMES(DSC);
DW_SYMNAMES(FORM);
DW_SYMNAMES(ID);
DW_SYMNAMES(INL);
DW_SYMNAMES(LNE);
DW_SYMNAMES(LNS);
DW_SYMNAMES(MACINFO);
DW_SYMNAMES(OP);
DW_SYMNAMES(VIRTUALITY);
DW_SYMNAMES(VIS);
DW_SYMNAMES(DS);
DW_SYMNAMES(END);
DW_SYMNAMES(UT);
#undef DW_SYMNAMES
#undef DW_SYMPREFIX

static const char *dwarf_get_symbol_name_slow(enum dwarf_symbol_namespace ns, dw_symval_t value);
static const char *dwarf_get_symbol_shortname_slow(enum dwarf_symbol_namespace ns, dw_symval_t value);

#define DW_SYMNSCASE_FAST(NAME) case DW_##NAME: return dw_##NAME##_names[value]
inline const char *dwarf_get_symbol_name(enum dwarf_symbol_namespace ns, dw_symval_t value)
{
    if (value < 256) {
        switch (ns) {
        DW_SYMNSCASE_FAST(AT);
        DW_SYMNSCASE_FAST(LANG);
        DW_SYMNSCASE_FAST(TAG);
        DW_SYMNSCASE_FAST(ORD);
        DW_SYMNSCASE_FAST(ACCESS);
        DW_SYMNSCASE_FAST(ATE);
        DW_SYMNSCASE_FAST(CC);
        DW_SYMNSCASE_FAST(CFA);
        DW_SYMNSCASE_FAST(CHILDREN);
        DW_SYMNSCASE_FAST(DSC);
        DW_SYMNSCASE_FAST(FORM);
        DW_SYMNSCASE_FAST(ID);
        DW_SYMNSCASE_FAST(INL);
        DW_SYMNSCASE_FAST(LNE);
        DW_SYMNSCASE_FAST(LNS);
        DW_SYMNSCASE_FAST(MACINFO);
        DW_SYMNSCASE_FAST(OP);
        DW_SYMNSCASE_FAST(VIRTUALITY);
        DW_SYMNSCASE_FAST(VIS);
        DW_SYMNSCASE_FAST(DS);
        DW_SYMNSCASE_FAST(END);
        DW_SYMNSCASE_FAST(UT);
        case DW_EH: break;
        }
    } else {
        return dwarf_get_symbol_name_slow(ns, value);
    }
    return NULL;
}
#undef DW_SYMNSCASE_FAST

#define DW_SYMNSCASE_FAST(NAME) case DW_##NAME: return dw_##NAME##_shortnames[value]
inline const char *dwarf_get_symbol_shortname(enum dwarf_symbol_namespace ns, dw_symval_t value)
{
    if (value < 256) {
        switch (ns) {
        DW_SYMNSCASE_FAST(AT);
        DW_SYMNSCASE_FAST(LANG);
        DW_SYMNSCASE_FAST(TAG);
        DW_SYMNSCASE_FAST(ORD);
        DW_SYMNSCASE_FAST(ACCESS);
        DW_SYMNSCASE_FAST(ATE);
        DW_SYMNSCASE_FAST(CC);
        DW_SYMNSCASE_FAST(CFA);
        DW_SYMNSCASE_FAST(CHILDREN);
        DW_SYMNSCASE_FAST(DSC);
        DW_SYMNSCASE_FAST(FORM);
        DW_SYMNSCASE_FAST(ID);
        DW_SYMNSCASE_FAST(INL);
        DW_SYMNSCASE_FAST(LNE);
        DW_SYMNSCASE_FAST(LNS);
        DW_SYMNSCASE_FAST(MACINFO);
        DW_SYMNSCASE_FAST(OP);
        DW_SYMNSCASE_FAST(VIRTUALITY);
        DW_SYMNSCASE_FAST(VIS);
        DW_SYMNSCASE_FAST(DS);
        DW_SYMNSCASE_FAST(END);
        DW_SYMNSCASE_FAST(UT);
        case DW_EH: break;
        }
    } else {
        return dwarf_get_symbol_shortname_slow(ns, value);
    }
    return NULL;
}
#undef DW_SYMNSCASE_FAST

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

inline const char *dwarf_get_symbol_name_slow(enum dwarf_symbol_namespace ns, dw_symval_t value)
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
inline const char *dwarf_get_symbol_shortname_slow(enum dwarf_symbol_namespace ns, dw_symval_t value)
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
