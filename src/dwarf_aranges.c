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

DWSTATIC(bool) dwarf_aranges_parseheader(struct dwarf *dwarf, dw_stream_t *stream, dwarf_aranges_t *aranges, struct dwarf_errinfo *errinfo)
{
    memset(aranges, 0x00, sizeof(dwarf_aranges_t));
    aranges->section_offset = dw_stream_tell(stream);
    aranges->length = dw_stream_getlen(stream, &aranges->dwarf64);
    aranges->version = dw_stream_get16(stream);
    assert(aranges->version == 2); /* FIXME: implement version 1 too, and error on 3+ */
    aranges->debug_info_offset = dw_stream_get32(stream);
    aranges->address_size = dw_stream_get8(stream);
    aranges->segment_size = dw_stream_get8(stream);
    assert(aranges->segment_size == 0); /* FIXME: We can probably support non-zero segment quite easily */
    assert(aranges->address_size == 8); /* FIXME: Why? */
    aranges->header_size = dw_stream_tell(stream) - aranges->section_offset;

    return true;
}
DWSTATIC(bool) dwarf_read_arange(struct dwarf *dwarf, dw_stream_t *stream, dwarf_aranges_t *aranges, dwarf_arange_t *arange, struct dwarf_errinfo *errinfo)
{
    int i;

    arange->segment = 0;
    arange->base = 0;
    arange->size = 0;
    /* FIXME: add dwarf->segment_size and read? */
    for (i=0; i < aranges->segment_size; i++) { dw_stream_get8(stream); }
    dw_stream_readahead(stream, 8);
    for (i=0; i < aranges->address_size - dw_addrsize(aranges->dwarf64); i++) { dw_stream_get8(stream); }
    arange->base = dw_stream_getaddr(stream, aranges->dwarf64);
    for (i=0; i < aranges->address_size - dw_addrsize(aranges->dwarf64); i++) { dw_stream_get8(stream); }
    arange->size = dw_stream_getaddr(stream, aranges->dwarf64);
    if (!arange->base && !arange->size) goto done;
    assert(arange->size != 0);

done:
    return true;
}
