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
#include <dweller/stream.h>

DWFUN(dw_u64_t)
dw_leb128_parse_unsigned(uint8_t *data, size_t size, int *shift_out)
{
    dw_u64_t result = 0;
    size_t i = 0;
    int shift = 0;
    for (;;) {
        dw_u8_t byte = data[i++];
        result |= (dw_u64_t)(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0)
            break;
        if (shift > 64 || shift + ffs(byte) > 64)
            goto fail;
        shift += 7;
    }

    if (shift_out) *shift_out = shift;
    return result;

fail:
    if (shift_out) *shift_out = -shift;
    return -1;
}
DWFUN(dw_u64_t)
dw_leb128_parse_signed(uint8_t *data, size_t size, int *shift_out)
{
    dw_u64_t result = 0;
    size_t i = 0;
    int shift = 0;
    dw_u8_t byte;

    do {
        byte = data[i++];
        result |= (dw_u64_t)(byte & 0x7f) << shift;
        if (shift > 64 || shift + ffs(byte) > 64)
            goto fail;
        shift += 7;
    } while ((byte & 0x80) != 0);

    if ((shift < 64) && (byte & 0x40))
        result |= -(1UL << shift); /* sign extend */

    if (shift_out) *shift_out = shift;
    return result;

fail:
    if (shift_out) *shift_out = -shift;
    return -1;
}

static inline uint8_t lastchunk(uint64_t value)
{
    while (value >> 7) value >>= 7;
    return value;
}
static inline int getshift(uint64_t value)
{
    int shift = 0;
    while (value) value >>= 7, shift += 7;
    return shift;
}

DWFUN(dw_i64_t)
dw_leb128_unsigned_to_signed(dw_u64_t value)
{
    /* This returns a best effort guess, however, it can't guarantee a valid reparse in every
     * situation, since some valid signed encodings are invalid unsigned:
     * 0xf4 0x00 means positive 0x74 when parsed as signed leb128
     * (the 0x00 at the end is required to avoid interpreting the number as negative),
     * but is an invalid encoding for 0x74 when parsed as unsigned leb128 (the 0x00 is redundant)
     * Only use this function for debugging!
     */
    int shift = getshift(value);
    int64_t result = value;
    if ((lastchunk(value) & 0x40) != 0) {
        result |= ~((1UL << shift) - 1);
    }
    return result;
}

DWFUN(dw_stream_t *)
dw_stream_init(dw_stream_t *stream, enum dwarf_section_namespace section_namespace, struct dwarf_section section, struct dwarf_section_provider *provider)
{
    memset(stream, 0x00, sizeof(dw_stream_t));
    if (provider == NULL) {
        stream->readahead = section.base;
        stream->readahead_size = section.size;
    } else {
        stream->provider = provider;
    }
    stream->section = section_namespace;
    return stream;
}

DWFUN(dw_stream_t)
dw_stream_new(enum dwarf_section_namespace section_namespace, struct dwarf_section section, struct dwarf_section_provider *provider)
{
    dw_stream_t result;
    return *dw_stream_init(&result, section_namespace, section, provider);
}

DWFUN(bool)
dw_stream_seek(dw_stream_t *stream, dw_i64_t off)
{
    stream->off = off;
    if (stream->provider) {
        int status;
        status = (stream->provider->seeker)((DW_SELF *)&stream->provider->seeker, stream->off, DW_SEEK_SET);
        if (status < 0) goto fail;
        status = (stream->provider->reader)((DW_SELF *)&stream->provider->reader, stream->buffer, sizeof(stream->buffer));
        if (status < 0) goto fail;
        stream->readahead = stream->buffer;
        stream->readahead_size = status;
        stream->readahead_offset = 0;
    } else {
        if (stream->off >= stream->readahead_size) goto fail;
        stream->readahead_offset = stream->off;
    }
    return true;

fail:
    stream->readahead_offset = -1;
    return false;
}

DWFUN(bool)
dw_stream_offset(dw_stream_t *stream, dw_i64_t off)
{
    return dw_stream_seek(stream, stream->off + off);
}
DWFUN(bool)
dw_stream_readahead(dw_stream_t *stream, int n)
{
    if (stream->readahead_offset == -1) return false;
    if (stream->readahead_size - stream->readahead_offset >= n) return true;
    if (stream->provider) {
        int status;
        memmove(stream->buffer, stream->buffer + stream->readahead_offset, sizeof(stream->buffer) - stream->readahead_offset);
        stream->readahead_offset = stream->readahead_size - stream->readahead_offset;
        status = (stream->provider->reader)(&stream->provider->reader, stream->buffer + stream->readahead_offset, sizeof(stream->buffer) - stream->readahead_offset);
        if (status < 0) goto fail;
        stream->readahead = stream->buffer;
        stream->readahead_size = stream->readahead_offset + status;
        stream->readahead_offset = 0;
    } else {
        if (stream->off >= stream->readahead_size) goto fail;
        stream->readahead_offset = stream->off;
    }
    /* TODO: Error if stream->readahead_size - stream->readahead_offset < n */
    return true;

fail:
    stream->readahead_offset = -1;
    return false;
}
DWFUN(bool)
dw_stream_read(dw_stream_t *stream, int n)
{
    if (!dw_stream_readahead(stream, n)) return false;
    stream->off += n;
    return true;
}

DWFUN(dw_stream_t *)
dw_stream_initfrom(dw_stream_t *stream, enum dwarf_section_namespace section_namespace, struct dwarf_section section, struct dwarf_section_provider *provider, dw_i64_t off)
{
    dw_stream_init(stream, section_namespace, section, provider);
    dw_stream_seek(stream, off);
    return stream;
}

DWFUN(bool)
dw_stream_isdone(dw_stream_t *stream)
{
    dw_stream_readahead(stream, 1); /* FIXME: This is a dirty, dirty trick */
    return stream->readahead_offset == -1;
}

DWFUN(dw_u8_t)
dw_stream_peak8(dw_stream_t *stream)
{
    dw_stream_readahead(stream, 1); /* TODO: Error on failure */
    return stream->readahead[stream->readahead_offset];
}
DWFUN(dw_u16_t)
dw_stream_peak16(dw_stream_t *stream)
{
    dw_u16_t result = 0;
    dw_stream_readahead(stream, 2);
    /* TODO: Big endian support */
    result |= stream->readahead[stream->readahead_offset + 0];
    result |= stream->readahead[stream->readahead_offset + 1] << 8;
    return result;
}
DWFUN(dw_u32_t)
dw_stream_peak32(dw_stream_t *stream)
{
    dw_u32_t result = 0;
    dw_stream_readahead(stream, 4);
    /* TODO: Big endian support */
    result |= stream->readahead[stream->readahead_offset + 0];
    result |= stream->readahead[stream->readahead_offset + 1] << 8;
    result |= stream->readahead[stream->readahead_offset + 2] << 16;
    result |= stream->readahead[stream->readahead_offset + 3] << 24;
    return result;
}
DWFUN(dw_u64_t)
dw_stream_peak64(dw_stream_t *stream)
{
    dw_u64_t result = 0;
    dw_stream_readahead(stream, 8);
    /* TODO: Big endian support */
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset + 0];
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset + 1] << 8;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset + 2] << 16;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset + 3] << 24;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset + 4] << 32;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset + 5] << 40;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset + 6] << 48;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset + 7] << 56;
    return result;
}
DWFUN(dw_u8_t)
dw_stream_get8(dw_stream_t *stream)
{
    dw_stream_read(stream, 1);
    return stream->readahead[stream->readahead_offset++];
}
DWFUN(dw_u16_t)
dw_stream_get16(dw_stream_t *stream)
{
    dw_u16_t result = 0;
    dw_stream_read(stream, 2);
    /* TODO: Big endian support */
    result |= (dw_u16_t)stream->readahead[stream->readahead_offset++];
    result |= (dw_u16_t)stream->readahead[stream->readahead_offset++] << 8;
    return result;
}
DWFUN(dw_u32_t)
dw_stream_get32(dw_stream_t *stream)
{
    dw_u32_t result = 0;
    dw_stream_read(stream, 4);
    /* TODO: Big endian support */
    result |= (dw_u32_t)stream->readahead[stream->readahead_offset++];
    result |= (dw_u32_t)stream->readahead[stream->readahead_offset++] << 8;
    result |= (dw_u32_t)stream->readahead[stream->readahead_offset++] << 16;
    result |= (dw_u32_t)stream->readahead[stream->readahead_offset++] << 24;
    return result;
}
DWFUN(dw_u64_t)
dw_stream_get64(dw_stream_t *stream)
{
    dw_u64_t result = 0;
    dw_stream_read(stream, 8);
    /* TODO: Big endian support */
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset++];
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset++] << 8;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset++] << 16;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset++] << 24;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset++] << 32;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset++] << 40;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset++] << 48;
    result |= (dw_u64_t)stream->readahead[stream->readahead_offset++] << 56;
    return result;
}
DWFUN(dw_u64_t)
dw_stream_getlen(dw_stream_t *stream, int *dwarf64)
{
    dw_u64_t result = dw_stream_get32(stream);
    if (result == 0xffffffff) {
        result = dw_stream_get64(stream);
        if (dwarf64) *dwarf64 = 64;
        return result;
    }
    if (result >= 0xffffff00) abort(); /* TODO: Failure */
    if (dwarf64) *dwarf64 = 32;
    return result;
}
DWFUN(dw_u64_t)
dw_stream_getleb128_unsigned(dw_stream_t *stream, int *shift_out)
{
    dw_u64_t result = 0;
    int shift = 0;
    for (;;) {
        dw_u8_t byte = dw_stream_get8(stream);
        result |= (byte & 0x7f) << shift;
        if ((byte & 0x80) == 0)
            break;
        if (shift > 64 || shift + ffs(byte) > 64)
            goto fail;
        shift += 7;
    }

    if (shift_out) *shift_out = shift;
    return result;

fail:
    if (shift_out) *shift_out = -shift;
    return -1;
}
DWFUN(dw_u64_t)
dw_stream_getleb128_signed(dw_stream_t *stream, int *shift_out)
{
    dw_u64_t result = 0;
    size_t i = 0;
    int shift = 0;
    dw_u8_t byte;

    do {
        byte = dw_stream_get8(stream);
        result |= (byte & 0x7f) << shift;
        if (shift > 64 || shift + ffs(byte) > 64)
            goto fail;
        shift += 7;
    } while ((byte & 0x80) != 0);

    if ((shift < 64) && (byte & 0x40))
        result |= -(1UL << shift); /* sign extend */

    if (shift_out) *shift_out = shift;
    return result;

fail:
    if (shift_out) *shift_out = -shift;
    return -1;
}
DWFUN(dw_u64_t)
dw_stream_peakaddr(dw_stream_t *stream, int addrsize)
{
    switch (addrsize) {
    case 8:
        return dw_stream_peak8(stream);
    case 16:
        return dw_stream_peak16(stream);
    case 32:
        return dw_stream_peak32(stream);
    case 64:
        return dw_stream_peak64(stream);
    }
    abort(); /* FIXME */
}
DWFUN(dw_u64_t)
dw_stream_getaddr(dw_stream_t *stream, int addrsize)
{
    switch (addrsize) {
    case 8:
        return dw_stream_get8(stream);
    case 16:
        return dw_stream_get16(stream);
    case 32:
        return dw_stream_get32(stream);
    case 64:
        return dw_stream_get64(stream);
    }
    abort(); /* FIXME */
}
