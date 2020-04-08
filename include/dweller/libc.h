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
#ifndef DWELLER_LIBC_H
#define DWELLER_LIBC_H
#include <stdio.h>
#include <stdlib.h>

static int dweller_libc_allocator_cb(dw_alloc_t *alloc,
        struct dwarf_alloc_req *req, void **pointer)
{
    if (!pointer) return -1;
    if (req->req_bytesize == 0) {
        free(*pointer);
        return 0;
    }
    void *newptr = realloc(*pointer, req->req_bytesize);
    if (!newptr) return -1;
    *pointer = newptr;
    return 0;
}
static dw_alloc_t dweller_libc_allocator = &dweller_libc_allocator_cb;

static int dweller_libc_stdin_reader_cb(dw_writer_t *writer,
        void *data, size_t size)
{
    fread(data, sizeof(char), size, stdin);
    return 0;
}
static dw_reader_t dweller_libc_stdin_reader = &dweller_libc_stdin_reader_cb;

static int dweller_libc_stdout_writer_cb(dw_writer_t *writer,
        const void *data, size_t size)
{
    fwrite(data, sizeof(char), size, stdout);
    return 0;
}
static dw_writer_t dweller_libc_stdout_writer = &dweller_libc_stdout_writer_cb;

static int dweller_libc_stderr_writer_cb(dw_writer_t *writer,
        const void *data, size_t size)
{
    fwrite(data, sizeof(char), size, stderr);
    return 0;
}
static dw_writer_t dweller_libc_stderr_writer = &dweller_libc_stderr_writer_cb;
#endif /* DWELLER_LIBC_H */
