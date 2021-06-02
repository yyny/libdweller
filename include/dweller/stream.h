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
#ifndef DWELLER_STREAM_H
#define DWELLER_STREAM_H
#include <dweller/core.h>
#include <dweller/dwarf.h>

#define DW_STREAM_BUFFERSIZE 8

#define dw_addrsize(addrsize) ((addrsize) / 8)

DWAPI(dw_u64_t)
dw_leb128_from(uint8_t *data, size_t size, int *shift_out);
DWAPI(dw_i64_t)
dw_leb128_tosigned(dw_u64_t val);

typedef struct dw_stream dw_stream_t;
struct dw_stream {
    enum dwarf_section_namespace section;
    struct dwarf_section_provider *provider;
    dw_i64_t off;
    const dw_u8_t *readahead;
    size_t readahead_size;
    size_t readahead_offset;
    size_t provider_offset;
    dw_u8_t buffer[DW_STREAM_BUFFERSIZE];
};

#define dw_stream_wasinit(stream) (!!((stream)->readahead))

DWAPI(dw_stream_t *)
dw_stream_init(dw_stream_t *stream, enum dwarf_section_namespace section_namespace, struct dwarf_section section, struct dwarf_section_provider *provider);
DWAPI(dw_stream_t *)
dw_stream_initfrom(dw_stream_t *stream, enum dwarf_section_namespace section_namespace, struct dwarf_section section, struct dwarf_section_provider *provider, dw_i64_t off);
DWAPI(dw_stream_t)
dw_stream_new(enum dwarf_section_namespace section_namespace, struct dwarf_section section, struct dwarf_section_provider *provider);

DWAPI(bool)
dw_stream_seek(dw_stream_t *stream, dw_i64_t off);
#define dw_stream_tell(stream) ((stream)->off)

DWAPI(bool)
dw_stream_isdone(dw_stream_t *stream);

DWAPI(bool)
dw_stream_offset(dw_stream_t *stream, dw_i64_t off);
DWAPI(bool)
dw_stream_readahead(dw_stream_t *stream, int n);
DWAPI(bool)
dw_stream_read(dw_stream_t *stream, int n);

DWAPI(dw_u8_t)
dw_stream_peak8(dw_stream_t *stream);
DWAPI(dw_u16_t)
dw_stream_peak16(dw_stream_t *stream);
DWAPI(dw_u32_t)
dw_stream_peak32(dw_stream_t *stream);
DWAPI(dw_u64_t)
dw_stream_peak64(dw_stream_t *stream);
DWAPI(dw_u8_t)
dw_stream_get8(dw_stream_t *stream);
DWAPI(dw_u16_t)
dw_stream_get16(dw_stream_t *stream);
DWAPI(dw_u32_t)
dw_stream_get32(dw_stream_t *stream);
DWAPI(dw_u64_t)
dw_stream_get64(dw_stream_t *stream);
DWAPI(dw_u64_t)
dw_stream_getlen(dw_stream_t *stream, int *dwarf64);
DWAPI(dw_u64_t)
dw_stream_getleb128_unsigned(dw_stream_t *stream, int *shift_out);
DWAPI(dw_u64_t)
dw_stream_getleb128_signed(dw_stream_t *stream, int *shift_out);
DWAPI(dw_u64_t)
dw_stream_peakaddr(dw_stream_t *stream, int addrsize);
DWAPI(dw_u64_t)
dw_stream_getaddr(dw_stream_t *stream, int addrsize);

#endif /* DWELLER_STREAM_H */
