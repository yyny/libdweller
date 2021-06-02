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

DWSTATIC(struct dwarf_abbreviation_table *) dwarf_find_abbreviation_table_at_offset(struct dwarf *dwarf, dw_off_t aboff)
{
    /* FIXME: We should use some kind of hashmap here... */
    struct dwarf_abbreviation_table *abtable = NULL;
    size_t i;
    for (i=0; i < dwarf->abbrev.num_tables; i++) {
        struct dwarf_abbreviation_table *table = &dwarf->abbrev.tables[i];
        if (table->debug_abbrev_offset == aboff) abtable = table;
        if (table->debug_abbrev_offset >= aboff) break;
    }
    return abtable;
}
DWFUN(dwarf_abbrev_t *) dwarf_abbrev_table_find_abbrev_from_code(struct dwarf *dwarf, struct dwarf_abbreviation_table *table, dw_symval_t abbrev_code)
{
    if (abbrev_code == 0) return NULL;
    if (abbrev_code > table->num_abbreviations) return NULL;
    if (table->is_sequential) return &table->abbreviations[abbrev_code - 1];
    else {
        /*
        TODO: bsearch if sorted, raw index if linear
        */
        size_t i;
        for (i=0; i < table->num_abbreviations; i++) {
            dwarf_abbrev_t *abbrev = &table->abbreviations[i];
            if (abbrev->abbrev_code == abbrev_code) {
                return abbrev;
            }
        }
    }
    return NULL;
}
