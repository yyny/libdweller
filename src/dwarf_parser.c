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

struct Parser {
  const uint8_t *base;
  size_t size;
  const uint8_t *cur;
};

#define done(parser) ((parser)->cur >= (parser)->base + (parser)->size)

static inline uint8_t  peak8(struct Parser *parser) { return *(uint8_t *)parser->cur; }
static inline uint16_t peak16(struct Parser *parser) { return *(uint16_t *)parser->cur; }
static inline uint32_t peak32(struct Parser *parser) { return *(uint32_t *)parser->cur; }
static inline uint64_t peak64(struct Parser *parser) { return *(uint64_t *)parser->cur; }
static inline uint8_t get8(struct Parser *parser) {
    uint8_t result = peak8(parser);
    parser->cur += sizeof(uint8_t);
    return result;
}
static inline uint16_t get16(struct Parser *parser) {
    uint16_t result = peak16(parser);
    parser->cur += sizeof(uint16_t);
    return result;
}
static inline uint32_t get32(struct Parser *parser) {
    uint32_t result = peak32(parser);
    parser->cur += sizeof(uint32_t);
    return result;
}
static inline uint64_t get64(struct Parser *parser) {
    uint64_t result = peak64(parser);
    parser->cur += sizeof(uint64_t);
    return result;
}
static inline uint64_t getvar64(struct Parser *parser, int *shift_out) {
    uint64_t result = 0;
    int shift = 0;
    while (true) {
        uint8_t byte = get8(parser);
        result |= (byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) break;
        if (shift > 64 || shift + ffs(byte) > 64) {
            if (shift_out) *shift_out = -shift;
            return -1;
        }
        shift += 7;
    }
    if (shift_out) *shift_out = shift;
    return result;
}
static inline int64_t var64_tosigned(uint64_t val) {
    int shift = 0;
    while (val >> (shift + 7)) shift += 7;
    if (shift < sizeof(uint64_t) * 8 && ((val >> shift) & 0x40)) {
        val |= -(1 << (shift + 7));
    }
    return (int64_t)val;
}
#define peakaddr(parser) (void *)peak64(parser)
#define getaddr(parser) (void *)get64(parser)
