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

static bool dwarf_cu_init(struct dwarf *dwarf, dwarf_cu_t *cu, struct dwarf_errinfo *errinfo)
{
    if (!dwarf_die_init(dwarf, &cu->unit.die, errinfo)) return false;
    cu->unit.type = DWARF_UNITTYPE_UNKNOWN;
    cu->unit.debug_abbrev_offset = 0;
    cu->unit.address_size = 0;
    cu->unit.version = 0;
    return true;
}
static dwarf_cu_t *dwarf_cu_new(struct dwarf *dwarf, struct dwarf_errinfo *errinfo)
{
    struct dwarf_alloc_req allocation_request = {
        sizeof(dwarf_cu_t),
        DWARF_ALLOC_STATIC,
        alignof(dwarf_cu_t),
        sizeof(dwarf_cu_t)
    };
    dwarf_cu_t *cu = NULL;
    dw_alloc_t *allocator = dwarf->allocator;
    if (!allocator) {
        error(no_allocator_error("failed to allocate compilation unit"));
    };
    int status = (*allocator)(allocator, &allocation_request, &cu);
    if (dw_unlikely(status < 0 || !dwarf)) {
        error(allocator_error(allocator, sizeof(dwarf_cu_t), cu, "failed to allocate compilation unit"));
    }
    return cu;
}
