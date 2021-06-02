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

DWSTATIC(bool) dwarf_unit_init(struct dwarf *dwarf, dwarf_unit_t *unit, struct dwarf_errinfo *errinfo)
{
    static dwarf_unit_t def_unit;
    *unit = def_unit;
    if (!dwarf_die_init(dwarf, &unit->die, errinfo)) return false;
    return true;
}
DWSTATIC(bool) dwarf_unit_parseheader(struct dwarf *dwarf, dw_stream_t *stream, dwarf_unit_t *unit, struct dwarf_errinfo *errinfo)
{
    unit->die.section_offset = dw_stream_tell(stream);
    unit->die.length = dw_stream_getlen(stream, &unit->dwarf64);
    unit->version = dw_stream_get16(stream);
    /*
    TODO: Actually check version
    assert(unit->version == 2 || unit->version == 4);

    NOTE: AFAIK, format is exactly the same, only form/value conventions are different
    The code as-is should be enough to just skip a section no matter the DWARF version
    */
    unit->type = DWARF_UNITTYPE_COMPILE; /* Version 4 and below only have compilation units */
    unit->debug_abbrev_offset = dw_stream_get32(stream);
    unit->address_size = dw_stream_get8(stream);
    assert(unit->address_size == 4 || unit->address_size == 8); /* FIXME: Check if we are 32 or 64 bit */
    struct dwarf_abbreviation_table *abtable = dwarf_find_abbreviation_table_at_offset(dwarf, unit->debug_abbrev_offset);
    if (!abtable) error(runtime_error("couldn't find an abbreviation table at offset %1 (for compilation unit at offset %2)", "II", unit->debug_abbrev_offset, unit->die.section_offset));
    unit->abbrev_table = abtable;
    unit->header_size = dw_stream_tell(stream) - unit->die.section_offset;

    return true;
}
