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
#ifndef DWELLER_CORE_H
#define DWELLER_CORE_H
#include <dweller/common.h>
#include <dweller/compiler.h>
#include <dweller/types.h>

/* We want to define the type of a pointer to a function and pass it as an
 * argument. However, we cannot use a typedef before it is defined, so we need
 * another, compatible type.
 *
 * C11 defines casting function pointers to void pointers and back in
 * Annex J. Subclause 5 "Common Extensions".
 * This doesn't mean that it's guaranteed to work, but it's a very good sign.
 * POSIX also defines this behaviour.
 *
 * However, we can do better.
 * Paragraph 6.3.2.3 "Pointers" states:
 * > A pointer to a function of one type may be converted to a pointer to a
 * > function of another type and back again; the result shall compare equal to
 * > the original pointer.  If a converted pointer is used to call a function
 * > whose type is not compatible with the referenced type, the behavior is
 * > undefined.
 *
 * Since all we really need is a _pointer_ to a pointer to a function,
 * and since we are only expecting a need to _compare_ the pointer values,
 * not actually _call_ them (as that may lead to infinite recursion),
 * this typedef will suffice.
 */
typedef int (*DW_SELF)();

/* A simple write callback with the same interface as write(2).
 */
typedef dw_mustuse int (*dw_writer_t)(DW_SELF *self, const void *data, size_t size) dw_nonnull(1, 2);
/* A simple read callback with the same interface as read(2).
 */
typedef dw_mustuse int (*dw_reader_t)(DW_SELF *self, void *data, size_t size) dw_nonnull(1, 2);
/* A simple seek callback with an interface similair to lseek(2).
 */
#define DW_SEEK_SET 0
#define DW_SEEK_CUR 1
#define DW_SEEK_END 2
typedef dw_i64_t (*dw_seeker_t)(DW_SELF *self, dw_i64_t off, int whence) dw_nonnull(1);
/* A callback for requesting a block of memory at a given position.
 * Pass `DW_UNMAP` to `whence` to unmap the mapped memory block.
 */
#define DW_UNMAP 0xffff
typedef dw_mustuse void *(*dw_mapper_t)(DW_SELF *self, dw_i64_t off, int whence, size_t length) dw_nonnull(1);
/* Defines the type of an allocation request.
 * The allocation request type determines what kind of memory is requested,
 * and how the memory will behave.
 */
enum dwarf_alloc_reqtype {
    /* @{DWARF_ALLOC_DYNAMIC} Request a block of memory whose size might need
     * to be grown or shrunk (`reallocated`) at some point.
     * It is undefined behaviour to reallocate a block of memory with a
     * different item alignment or item size than the one requested
     * when the memory block was first created.
     * Allocators may return a pointer to a block of memory even if they are
     * not able to reallocate it.
     * In this case the allocator will fail when a reallocation is requested.
     */
    DWARF_ALLOC_DYNAMIC = 0,
    /* @{DWARF_ALLOC_STATIC} Request a block of memory whose size will never
     * have to change for the duration of it's existance.
     * Trying to grow or shrink a pointer returned from a request of this
     * type is undefined behaviour.
     */
    DWARF_ALLOC_STATIC,
    /* @{DWARF_ALLOC_BUFFER} Request for a block of memory whose size is not
     * critical for the execution of the program to continue.
     * The contents of the buffer must only remain valid until the next
     * allocation request made to this allocator.
     * A request of this type may return a static pointer to a
     * location in program memory (or thread-local storage) shared between
     * all requests of this type.
     * Additionally, The allocator might return a pointer to a block of memory
     * that is smaller than what was requested.
     * A response to an allocation of this type _must_ update the
     * `req_bytesize` value in the `struct dwarf_alloc_req` if the returned
     * block of memory is smaller than what was requested.
     * The input value of the `pointer` argument should be ignored for a
     * request of this type, although a compliant user should
     * still set it to `NULL`.
     * Every request of this type should still be followed by a deallocation
     * request, unless specified otherwise by the allocator, and deallocation
     * requests must not invalidate (the contents of) any other memory block
     * returned by this allocator.
     */
    DWARF_ALLOC_BUFFER,
    /*
     * @{DWARF_ALLOC_OBSTACK} Request for a block of memory that must be
     * deallocated before a new allocation request can be made.
     * Reallocation requests of the latest allocated object might still occur.
     */
    DWARF_ALLOC_OBSTACK,
    /* @{DWARF_ALLOC_EXTENDED} Flag reserved for complicated allocation
     * requests that cannot be described using just the members
     * in a `struct dwarf_alloc_req`.
     * In this case, there will be a `size_t` after the request
     * specifying the size of the extended request, followed by that many
     * bytes of extended request data.
     * `libdweller` never sends a request of this type, it is only here for
     * compatability with other allocation implementations.
     */
    DWARF_ALLOC_EXTENDED = 0x80
};
/* Defines an allocation request in terms of the number of bytes to allocate,
 * the type of memory block requested, the item alignment the block must have,
 * and the size of each item that will be written to the memory block.
 * Unless this is a deallocation request, the request object must be writable,
 * and all members must contain valid values (unless negotiated otherwise
 * with the allocator).
 * The allocator may want to write to the request to reflect the properties of
 * the memory block that was actually allocated.
 */
struct dwarf_alloc_req {
    /* @{req_bytesize} The requested size of the memory block to be returned
     * by the allocator as the `pointer` argument in bytes.
     * Unless specified by the allocator, it may write to this location
     * to return the number of bytes actually allocated.
     * For a deallocation request, this must be zero.
     */
    size_t req_bytesize;
    /* @{req_type} The type of allocation request for this allocation,
     * as defined in `enum dwarf_alloc_reqtype`.
     * An allocator may ignore this value, and always default to the
     * `DWARF_ALLOC_DYNAMIC` behaviour.
     */
    dw_u8_t req_type;
    /* @{req_itemalign} The alignment required for the items that will be
     * stored in the pointer returned by this allocator, or 0 if any alignment
     * will suffice.
     */
    dw_u8_t req_itemalign;
    /* @{req_itemsize} The byte sizeof the items that will be stored in the
     * pointer returned by this allocator, or 0 if it's not known
     * or not constant.
     * The allocator may use this for optimization and debugging purposes.
     */
    dw_u16_t req_itemsize;
};
/* @{self} A pointer to the function-pointer of this allocator itself.
 * You can define a struct with a `dw_alloc_t` as it's first member and
 * allocator-specific data in the following members.
 * You can then safely cast this pointer to the defined struct to obtain
 * access to the data, the C standard guarantees this will work.
 * @{req} A (writable) pointer to the allocation request.
 * A user might negotiate with the allocator to not write to the request
 * object, so they can reuse a single object.
 * A typical allocator might write the number of bytes that were actually
 * allocated to the request object, if they are different from what was
 * requested.
 * If this is `NULL`, indicates that the user will no longer use any of the
 * memory blocks previously allocated by this allocator, and that they will
 * never call this allocator again.
 * @{pointer} A reference to a pointer to a memory block previously returned
 * by this allocator that is requested to be reallocated.
 * A pointer to the new block is written back.
 * If the pointer value is `NULL`, a new memory block should be allocated.
 * A deallocation request with a pointer value of `NULL`
 * (effectively a `free(NULL)`) should be valid.
 * An allocator does not have to set the pointer to `NULL` on deallocation,
 * although this is recommended.
 * Passing `pointer` itself as `NULL` can be used to request for internal
 * allocator state (such as perhaps total memory usage or available memory).
 * In this case, the state request is put in the `req` parameter, and the
 * result is returned as an integer or written to the `req` parameter.
 * Allocators that do not support this must return `-1`.
 * If both `req` and `pointer` are `NULL`, no further allocations will be made
 * for this allocator.
 * The allocator may deallocate any internal state, and does not have to
 * respond to further requests with the behaviour defined above.
 * @pointer Pointer to pointer (Typed as a void pointer to avoid warnings)
 */
typedef dw_mustuse int (*dw_alloc_t)(DW_SELF *self, struct dwarf_alloc_req *req, void **pointer) dw_nonnull(1);

#endif /* DWELLER_CORE_H */
