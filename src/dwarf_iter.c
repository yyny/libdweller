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

DWFUN(void *) dwarf_iter_next(dwarf_iter_t *iter)
{
    if (!iter) return NULL;
    if (!iter->next) return NULL;
    return iter->next(iter);
}

struct dwarf_aranges_iter {
    dwarf_iter_t iter;
    dw_stream_t stream;
    dwarf_aranges_t aranges;
    dw_i64_t offset;
};
struct dwarf_arange_iter {
    dwarf_iter_t iter;
    dw_stream_t stream;
    dwarf_arange_t arange;
    dwarf_aranges_t *aranges;
};
struct dwarf_unit_iter {
    dwarf_iter_t iter;
    dw_stream_t stream;
    dwarf_unit_t unit;
    dw_i64_t offset;
};
struct dwarf_attr_iter {
    dwarf_iter_t iter;
    dwarf_unit_t *unit;
    dwarf_die_t *die;
    dw_stream_t *stream;
    dw_stream_t abstream;
    dwarf_attr_t attr;
    dw_stream_t stream_holder;
};
struct dwarf_die_iter {
    dwarf_iter_t iter;
    dw_stream_t stream;
    dwarf_unit_t unit;
    dwarf_die_t die;
    dwarf_attr_iter_t attrs;
};
struct dwarf_line_program_iter {
    dwarf_iter_t iter;
    dw_stream_t stream;
    dw_i64_t offset;
    struct dwarf_line_program line_program;
};
struct dwarf_line_row_iter {
    dwarf_iter_t iter;
    dw_stream_t stream;
    struct dwarf_line_program *line_program;
    struct dwarf_line_program_state state;
    struct dwarf_line_program_state state_out;
};

#define error_iter(ERROR) do { if (viter->errinfo) { *viter->errinfo = ERROR; } goto done; } while (0)

DWSTATIC(void *) dwarf_aranges_iter_next(dwarf_iter_t *viter)
{
    dwarf_aranges_iter_t *iter = (dwarf_aranges_iter_t *)viter;
    dw_stream_seek(&iter->stream, iter->offset);
    if (dw_stream_isdone(&iter->stream)) goto done;
    dwarf_aranges_parseheader(viter->dwarf, &iter->stream, &iter->aranges, viter->errinfo);
    dw_i64_t header_length_size = dwarf_header_length_size(iter->aranges.dwarf64); /* The size of the length field of the header */
    iter->offset = dw_stream_tell(&iter->stream) - (iter->aranges.header_size - header_length_size) + iter->aranges.length;

    return &iter->aranges;

done:
    viter->next = NULL;
    return NULL;
}
DWSTATIC(void *) dwarf_arange_iter_next(dwarf_iter_t *viter)
{
    dwarf_arange_iter_t *iter = (dwarf_arange_iter_t *)viter;
    if (dw_stream_isdone(&iter->stream)) goto done;
    if (!dwarf_read_arange(viter->dwarf, &iter->stream, iter->aranges, &iter->arange, viter->errinfo)) goto done;
    if (iter->arange.base == 0 && iter->arange.size == 0) goto done;

    return &iter->arange;

done:
    viter->next = NULL;
    return NULL;
}
DWSTATIC(void *) dwarf_unit_iter_next(dwarf_iter_t *viter)
{
    dwarf_unit_iter_t *iter = (dwarf_unit_iter_t *)viter;
    dw_stream_seek(&iter->stream, iter->offset);
    if (dw_stream_isdone(&iter->stream)) goto done;
    dwarf_unit_parseheader(viter->dwarf, &iter->stream, &iter->unit, viter->errinfo); // TODO: Error handling
    dw_i64_t header_length_size = dwarf_header_length_size(iter->unit.dwarf64); /* The size of the length field of the header */
    iter->offset = dw_stream_tell(&iter->stream) - (iter->unit.header_size - header_length_size) + iter->unit.die.length;
    return &iter->unit;

done:
    viter->next = NULL;
    return NULL;
}
DWSTATIC(void *) dwarf_die_iter_next(dwarf_iter_t *viter)
{
    dwarf_die_iter_t *iter = (dwarf_die_iter_t *)viter;
    if (dw_stream_isdone(&iter->stream)) goto done;
    while (dwarf_next(&iter->attrs));
    if (iter->die.has_children) iter->die.depth++;
again:
    if (iter->die.depth == 0 && dw_stream_tell(&iter->stream) != iter->unit.die.section_offset + iter->unit.header_size) goto done;
    if (dw_stream_peak8(&iter->stream) == 0x00) {
        if (iter->die.depth == 0) {
            /* Empty compilation unit */
            /* TODO: Should this be an error? */
            goto done;
        }
        dw_stream_get8(&iter->stream);
        iter->die.depth--;
        goto again;
    }
    iter->die.section = iter->unit.die.section;
    iter->die.section_offset = dw_stream_tell(&iter->stream);
    iter->die.abbrev_code = dw_stream_getleb128_unsigned(&iter->stream, NULL);
    if (!dwarf_attr_iter_from(viter->dwarf, &iter->attrs, iter, viter->errinfo)) goto done;
    iter->die.tag = dw_stream_getleb128_unsigned(&iter->attrs.abstream, NULL);
    iter->die.has_children = dw_stream_get8(&iter->attrs.abstream);
    return &iter->die;

done:
    viter->next = NULL;
    return NULL;
}
DWSTATIC(void *) dwarf_attr_iter_next(dwarf_iter_t *viter)
{
    dwarf_attr_iter_t *iter = (dwarf_attr_iter_t *)viter;
    if (dw_stream_isdone(iter->stream)) goto done;
    if (dw_stream_isdone(&iter->abstream)) goto done;
    if (!dw_stream_peak16(&iter->abstream)) goto done;
    iter->attr.name = dw_stream_getleb128_unsigned(&iter->abstream, NULL);
    iter->attr.form = dw_stream_getleb128_unsigned(&iter->abstream, NULL);
    dwarf_read_attr(viter->dwarf, iter->stream, &iter->attr);
    return &iter->attr;

done:
    viter->next = NULL;
    return NULL;
}
DWSTATIC(void *) dwarf_line_program_iter_next(dwarf_iter_t *viter)
{
    dwarf_line_program_iter_t *iter = (dwarf_line_program_iter_t *)viter;
    dw_stream_seek(&iter->stream, iter->offset);
    if (dw_stream_isdone(&iter->stream)) goto done;
    dwarf_read_line_program_header(viter->dwarf, &iter->stream, &iter->line_program, viter->errinfo);
    dw_i64_t header_length_size = dwarf_header_length_size(iter->line_program.dwarf64); /* The size of the length field of the header */
    iter->offset = dw_stream_tell(&iter->stream) - (iter->line_program.header_size - header_length_size) + iter->line_program.length;
    return &iter->line_program;

done:
    viter->next = NULL;
    return NULL;
}
DWSTATIC(void *) dwarf_line_row_iter_next(dwarf_iter_t *viter)
{
    dwarf_line_row_iter_t *iter = (dwarf_line_row_iter_t *)viter;
    if (dw_stream_isdone(&iter->stream)) goto done;
    if (dw_stream_tell(&iter->stream) >= iter->line_program->section_offset + iter->line_program->length + dwarf_header_length_size(iter->line_program->dwarf64)) goto done;
    dwarf_read_line_program_row(viter->dwarf, &iter->stream, iter->line_program, &iter->state, &iter->state_out, viter->errinfo);
    return &iter->state_out;

done:
    viter->next = NULL;
    return NULL;
}

DWFUN(bool) dwarf_aranges_iter_init(struct dwarf *dwarf, dwarf_aranges_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    dw_stream_init(&iter->stream, DWARF_SECTION_ARANGES, dwarf->aranges.section, dwarf->aranges.section_provider);
    iter->iter.next = dwarf_aranges_iter_next;
    iter->iter.dwarf = dwarf;
    iter->iter.dwarf = errinfo;
    memset(&iter->aranges, 0x00, sizeof(dwarf_aranges_t)); // TODO: dwarf_aranges_init
    iter->offset = 0;
    return true;
}
DWFUN(bool) dwarf_arange_iter_init(struct dwarf *dwarf, dwarf_arange_iter_t *iter, dwarf_aranges_t *aranges, struct dwarf_errinfo *errinfo)
{
    dw_stream_initfrom(&iter->stream, DWARF_SECTION_ARANGES, dwarf->aranges.section, dwarf->aranges.section_provider, aranges->section_offset + aranges->header_size);
    iter->iter.next = dwarf_arange_iter_next;
    iter->iter.dwarf = dwarf;
    iter->iter.dwarf = errinfo;
    iter->aranges = aranges;
    memset(&iter->arange, 0x00, sizeof(dwarf_arange_t)); // TODO: dwarf_arange_init
    return true;
}
DWFUN(bool) dwarf_unit_iter_init(struct dwarf *dwarf, dwarf_unit_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    dw_stream_init(&iter->stream, DWARF_SECTION_INFO, dwarf->info.section, dwarf->info.section_provider);
    iter->iter.next = dw_stream_isdone(&iter->stream) ? NULL : dwarf_unit_iter_next;
    iter->iter.dwarf = dwarf;
    iter->iter.errinfo = errinfo;
    dwarf_unit_init(dwarf, &iter->unit, errinfo); // TODO: Error checking
    iter->unit.die.section = DWARF_SECTION_INFO;
    iter->offset = 0;
    return true;
}
DWFUN(bool) dwarf_die_iter_init(struct dwarf *dwarf, dwarf_die_iter_t *iter, dwarf_unit_t *unit, struct dwarf_errinfo *errinfo)
{
    memset(iter, 0x00, sizeof(dwarf_die_iter_t));
    dw_stream_initfrom(&iter->stream, DWARF_SECTION_INFO, dwarf->info.section, dwarf->info.section_provider, unit->die.section_offset + unit->header_size);
    iter->iter.next = dw_stream_isdone(&iter->stream) ? NULL : dwarf_die_iter_next;
    iter->iter.dwarf = dwarf;
    iter->iter.errinfo = errinfo;
    dwarf_die_init(dwarf, &iter->die, errinfo);  // TODO: Error checking
    iter->die.section = DWARF_SECTION_INFO;
    iter->unit = *unit;
    return true;
}
DWFUN(bool) dwarf_attr_iter_initwith(struct dwarf *dwarf, dwarf_attr_iter_t *iter, dwarf_unit_t *unit, dwarf_die_t *die, dw_stream_t *stream, struct dwarf_errinfo *errinfo)
{
    memset(iter, 0x00, sizeof(dwarf_attr_iter_t));
    iter->iter.dwarf = dwarf;
    iter->iter.errinfo = errinfo;
    iter->unit = unit;
    iter->die = die;
    if (!stream) {
        iter->stream = &iter->stream_holder;
        dw_stream_initfrom(iter->stream, DWARF_SECTION_INFO, dwarf->info.section, dwarf->info.section_provider, die->section_offset); // TODO: Error checking
    } else {
        iter->stream = stream;
    }
    iter->iter.next = dw_stream_isdone(iter->stream) ? NULL : dwarf_attr_iter_next;
    dwarf_abbrev_t *abbrev = dwarf_abbrev_table_find_abbrev_from_code(dwarf, unit->abbrev_table, die->abbrev_code);
    if (!abbrev) error(runtime_error("couldn't find an abbreviation table at offset %1 (for compilation unit at offset %2)", "II", iter->unit->debug_abbrev_offset, iter->unit->die.section_offset)); // TODO: Better error message
    dw_stream_initfrom(&iter->abstream, DWARF_SECTION_ABBREV, dwarf->abbrev.section, dwarf->abbrev.section_provider, abbrev->offset);
    /* Skip over the abbreviation code */
    check(iter->die->abbrev_code == dw_stream_getleb128_unsigned(&iter->abstream, NULL));
    if (!stream) {
        check(iter->die->tag == dw_stream_getleb128_unsigned(&iter->abstream, NULL));
        check(iter->die->has_children == dw_stream_get8(&iter->abstream));
    }
    return true;
}
DWFUN(bool) dwarf_attr_iter_init(struct dwarf *dwarf, dwarf_attr_iter_t *iter, dwarf_unit_t *unit, dwarf_die_t *die, struct dwarf_errinfo *errinfo)
{
    return dwarf_attr_iter_initwith(dwarf, iter, unit, die, NULL, errinfo);
}
DWFUN(bool) dwarf_attr_iter_from(struct dwarf *dwarf, dwarf_attr_iter_t *iter, dwarf_die_iter_t *die_iter, struct dwarf_errinfo *errinfo)
{
    return dwarf_attr_iter_initwith(dwarf, iter, &die_iter->unit, &die_iter->die, &die_iter->stream, errinfo);
}
DWFUN(bool) dwarf_line_program_iter_init(struct dwarf *dwarf, dwarf_line_program_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    dw_stream_init(&iter->stream, DWARF_SECTION_LINE, dwarf->line.section, dwarf->line.section_provider);
    iter->iter.next = dw_stream_isdone(&iter->stream) ? NULL : dwarf_line_program_iter_next;
    iter->iter.dwarf = dwarf;
    iter->iter.errinfo = errinfo;
    iter->offset = 0;
    dwarf_line_program_init(dwarf, &iter->line_program);
    return true;
}
DWFUN(bool) dwarf_line_row_iter_init(struct dwarf *dwarf, dwarf_line_row_iter_t *iter, struct dwarf_line_program *line_program, struct dwarf_errinfo *errinfo)
{
    dw_stream_initfrom(&iter->stream, DWARF_SECTION_LINE, dwarf->line.section, dwarf->line.section_provider, line_program->section_offset + line_program->header_size);
    iter->iter.next = dw_stream_isdone(&iter->stream) ? NULL : dwarf_line_row_iter_next;
    iter->iter.dwarf = dwarf;
    iter->iter.errinfo = errinfo;
    iter->line_program = line_program;
    dwarf_line_program_state_init(dwarf, line_program, &iter->state);
    dwarf_line_program_state_init(dwarf, line_program, &iter->state_out);
    return true;
}

DWFUN(bool) dwarf_aranges_iter_fini(struct dwarf *dwarf, dwarf_aranges_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    return true;
}
DWFUN(bool) dwarf_arange_iter_fini(struct dwarf *dwarf, dwarf_arange_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    return true;
}
DWFUN(bool) dwarf_unit_iter_fini(struct dwarf *dwarf, dwarf_unit_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    return true;
}
DWFUN(bool) dwarf_die_iter_fini(struct dwarf *dwarf, dwarf_die_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    return true;
}
DWFUN(bool) dwarf_attr_iter_fini(struct dwarf *dwarf, dwarf_attr_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    return true;
}
DWFUN(bool) dwarf_line_program_iter_fini(struct dwarf *dwarf, dwarf_line_program_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    return true;
}
DWFUN(bool) dwarf_line_row_iter_fini(struct dwarf *dwarf, dwarf_line_row_iter_t *iter, struct dwarf_errinfo *errinfo)
{
    return true;
}

DWFUN(dwarf_aranges_iter_t *) dwarf_aranges_iter(struct dwarf *dwarf, dwarf_aranges_iter_t *saved, struct dwarf_errinfo *errinfo)
{
    dwarf_aranges_iter_t *iter = saved;
    if (saved == NULL) {
        iter = dw_malloc(dwarf, sizeof(dwarf_aranges_iter_t));
    }
    if (!dwarf_aranges_iter_init(dwarf, iter, errinfo)) goto fail;
    return iter;

fail:
    if (saved == NULL) {
        dw_free(dwarf, iter);
    }
    return NULL;
}
DWFUN(dwarf_arange_iter_t *) dwarf_arange_iter(struct dwarf *dwarf, dwarf_arange_iter_t *saved, dwarf_aranges_t *aranges, struct dwarf_errinfo *errinfo)
{
    dwarf_arange_iter_t *iter = saved;
    if (saved == NULL) {
        iter = dw_malloc(dwarf, sizeof(dwarf_arange_iter_t));
    }
    if (!dwarf_arange_iter_init(dwarf, iter, aranges, errinfo)) goto fail;
    return iter;

fail:
    if (saved == NULL) {
        dw_free(dwarf, iter);
    }
    return NULL;
}
DWFUN(dwarf_unit_iter_t *) dwarf_unit_iter(struct dwarf *dwarf, dwarf_unit_iter_t *saved, struct dwarf_errinfo *errinfo)
{
    dwarf_unit_iter_t *iter = saved;
    if (saved == NULL) {
        iter = dw_malloc(dwarf, sizeof(dwarf_unit_iter_t));
    }
    if (!dwarf_unit_iter_init(dwarf, iter, errinfo)) goto fail;
    return iter;

fail:
    if (saved == NULL) {
        dw_free(dwarf, iter);
    }
    return NULL;
}
DWFUN(dwarf_die_iter_t *) dwarf_unit_entry_iter(struct dwarf *dwarf, dwarf_die_iter_t *saved, dwarf_unit_t *unit, struct dwarf_errinfo *errinfo)
{
    dwarf_die_iter_t *iter = saved;
    if (saved == NULL) {
        iter = dw_malloc(dwarf, sizeof(dwarf_die_iter_t));
    }
    if (!dwarf_die_iter_init(dwarf, iter, unit, errinfo)) goto fail;
    return iter;

fail:
    if (saved == NULL) {
        dw_free(dwarf, iter);
    }
    return NULL;
}
DWFUN(dwarf_attr_iter_t *) dwarf_unit_entry_attr_iter(struct dwarf *dwarf, dwarf_attr_iter_t *saved, dwarf_die_iter_t *die_iter, struct dwarf_errinfo *errinfo)
{
    DW_USE(saved);

    return &die_iter->attrs;
}
DWFUN(dwarf_attr_iter_t *) dwarf_die_attr_iter(struct dwarf *dwarf, dwarf_attr_iter_t *saved, dwarf_unit_t *unit, dwarf_die_t *die, struct dwarf_errinfo *errinfo)
{
    dwarf_attr_iter_t *iter = saved;
    if (saved == NULL) {
        iter = dw_malloc(dwarf, sizeof(dwarf_attr_iter_t));
    }
    if (!dwarf_attr_iter_init(dwarf, iter, unit, die, errinfo)) goto fail;
    return iter;

fail:
    if (saved == NULL) {
        dw_free(dwarf, iter);
    }
    return NULL;
}
DWFUN(dwarf_line_program_iter_t *) dwarf_line_program_iter(struct dwarf *dwarf, dwarf_line_program_iter_t *saved, struct dwarf_errinfo *errinfo)
{
    dwarf_line_program_iter_t *iter = saved;
    if (saved == NULL) {
        iter = dw_malloc(dwarf, sizeof(dwarf_line_program_iter_t));
    }
    if (!dwarf_line_program_iter_init(dwarf, iter, errinfo)) goto fail;
    return iter;

fail:
    if (saved == NULL) {
        dw_free(dwarf, iter);
    }
    return NULL;
}
DWFUN(dwarf_line_row_iter_t *) dwarf_line_row_iter(struct dwarf *dwarf, dwarf_line_row_iter_t *saved, struct dwarf_line_program *line_program, struct dwarf_errinfo *errinfo)
{
    dwarf_line_row_iter_t *iter = saved;
    if (saved == NULL) {
        iter = dw_malloc(dwarf, sizeof(dwarf_line_row_iter_t));
    }
    if (!dwarf_line_row_iter_init(dwarf, iter, line_program, errinfo)) goto fail;
    return iter;

fail:
    if (saved == NULL) {
        dw_free(dwarf, iter);
    }
    return NULL;
}

DWFUN(void) dwarf_aranges_iter_free(struct dwarf *dwarf, dwarf_aranges_iter_t *iter)
{
    dwarf_aranges_iter_fini(dwarf, iter, NULL);
    dw_free(dwarf, iter);
}
DWFUN(void) dwarf_arange_iter_free(struct dwarf *dwarf, dwarf_arange_iter_t *iter)
{
    dwarf_arange_iter_fini(dwarf, iter, NULL);
    dw_free(dwarf, iter);
}
DWFUN(void) dwarf_unit_iter_free(struct dwarf *dwarf, dwarf_unit_iter_t *iter)
{
    dwarf_unit_iter_fini(dwarf, iter, NULL);
    dw_free(dwarf, iter);
}
DWFUN(void) dwarf_die_iter_free(struct dwarf *dwarf, dwarf_die_iter_t *iter)
{
    dwarf_die_iter_fini(dwarf, iter, NULL);
    dw_free(dwarf, iter);
}
DWFUN(void) dwarf_attr_iter_free(struct dwarf *dwarf, dwarf_attr_iter_t *iter)
{
    if (dw_stream_wasinit(&iter->stream_holder)) {
        dwarf_attr_iter_fini(dwarf, iter, NULL);
        dw_free(dwarf, iter);
    }
}
DWFUN(void) dwarf_line_program_iter_free(struct dwarf *dwarf, dwarf_line_program_iter_t *iter)
{
    dwarf_line_program_iter_fini(dwarf, iter, NULL);
    dw_free(dwarf, iter);
}
DWFUN(void) dwarf_line_row_iter_free(struct dwarf *dwarf, dwarf_line_row_iter_t *iter)
{
    dwarf_line_row_iter_fini(dwarf, iter, NULL);
    dw_free(dwarf, iter);
}
