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

static bool dwarf_die_init(struct dwarf *dwarf, dwarf_die_t *die, struct dwarf_errinfo *errinfo)
{
    memset(die, 0x00, sizeof(dwarf_die_t));
    return true;
}
static dwarf_die_t *dwarf_die_new(struct dwarf *dwarf, struct dwarf_errinfo *errinfo)
{
    struct dwarf_alloc_req allocation_request = {
        sizeof(dwarf_die_t),
        DWARF_ALLOC_STATIC,
        alignof(dwarf_die_t),
        sizeof(dwarf_die_t)
    };
    dwarf_die_t *die = NULL;
    dw_alloc_t *allocator = dwarf->allocator;
    if (!allocator) {
        error(no_allocator_error("failed to allocate compilation unit"));
    };
    int status = (*allocator)(allocator, &allocation_request, (void **)&die);
    if (dw_unlikely(status < 0 || !dwarf)) {
        error(allocator_error(allocator, sizeof(dwarf_die_t), die, "failed to allocate compilation unit"));
    }
    return die;
}
