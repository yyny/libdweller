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
#ifndef DWELLER_UTIL_H
#define DWELLER_UTIL_H
#include <dweller/dwarf.h>

static void *dw_malloc(struct dwarf *dwarf, size_t capacity)
{
    void *ptr = NULL;
    struct dwarf_alloc_req req = {
        capacity,
        DWARF_ALLOC_DYNAMIC,
        DW_MAXALIGN,
        1
    };

    dw_alloc_t *allocator = dwarf->allocator;
    (void)(*allocator)(allocator, &req, &ptr);

    return ptr;
}
static void *dw_realloc(struct dwarf *dwarf, void *ptr, size_t capacity)
{
    struct dwarf_alloc_req req = {
        capacity,
        DWARF_ALLOC_DYNAMIC,
        DW_MAXALIGN,
        1
    };

    dw_alloc_t *allocator = dwarf->allocator;
    (void)(*allocator)(allocator, &req, &ptr);

    return ptr;
}
static void dw_free(struct dwarf *dwarf, void *ptr)
{
    static struct dwarf_alloc_req req = {
        0,
        DWARF_ALLOC_DYNAMIC,
        DW_MAXALIGN,
        1
    };

    dw_alloc_t *allocator = dwarf->allocator;
    (void)(*allocator)(allocator, &req, &ptr);
}
#endif /* DWELLER_UTIL_H */
