/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 *
 * Copyright (c) 2024-present, Tampere University, ITU/ISO/IEC, project contributors
 * All rights reserved.
 ****************************************************************************/

#include "v3cbitstream.hpp"

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>

namespace uvgv3cbitstream {

namespace {

unsigned uvg_math_floor_log2(unsigned value) {
    assert(value > 0);
    unsigned result = 0;
    for (size_t i = 4;; --i) {
        const unsigned bits = 1ULL << i;
        const unsigned shift = value >= (1U << bits) ? bits : 0;
        result += shift;
        value >>= shift;
        if (i == 0) {
            break;
        }
    }
    return result;
}

}  // namespace

void uvg_bitstream_init(bitstream_t* const stream) { memset(stream, 0, sizeof(bitstream_t)); }

uvg_data_chunk* uvg_bitstream_alloc_chunk() {
    auto* chunk = static_cast<uvg_data_chunk*>(malloc(sizeof(uvg_data_chunk)));
    if (chunk != nullptr) {
        chunk->len = 0;
        chunk->next = nullptr;
    }
    return chunk;
}

void uvg_bitstream_free_chunks(uvg_data_chunk* chunk) {
    while (chunk != nullptr) {
        uvg_data_chunk* next = chunk->next;
        free(chunk);
        chunk = next;
    }
}

void uvg_bitstream_writebyte(bitstream_t* const stream, const uint8_t byte) {
    assert(stream->cur_bit == 0);
    if (stream->last == nullptr || stream->last->len == UVG_DATA_CHUNK_SIZE) {
        uvg_data_chunk* new_chunk = uvg_bitstream_alloc_chunk();
        assert(new_chunk);
        if (stream->first == nullptr) {
            stream->first = new_chunk;
        }
        if (stream->last != nullptr) {
            stream->last->next = new_chunk;
        }
        stream->last = new_chunk;
    }
    stream->last->data[stream->last->len] = byte;
    stream->last->len += 1;
    stream->len += 1;
}

void uvg_bitstream_put(bitstream_t* const stream, const uint32_t data, uint8_t bits) {
    while (bits-- != 0) {
        stream->data <<= 1U;
        if ((data & 1UL <<bits) != 0U) {
            stream->data |= 1U;
        }
        stream->cur_bit++;
        if (stream->cur_bit == 8) {
            stream->cur_bit = 0;
            uvg_bitstream_writebyte(stream, stream->data);
        }
    }
}

uvg_data_chunk* uvg_bitstream_take_chunks(bitstream_t* const stream) {
    assert(stream->cur_bit == 0);
    uvg_data_chunk* chunks = stream->first;
    stream->first = stream->last = nullptr;
    stream->len = 0;
    return chunks;
}

void uvg_bitstream_finalize(bitstream_t* const stream) {
    uvg_bitstream_free_chunks(stream->first);
    delete stream;
}

void uvg_bitstream_clear(bitstream_t* const stream) {
    uvg_bitstream_free_chunks(stream->first);
    uvg_bitstream_init(stream);
}

uint64_t uvg_bitstream_tell(const bitstream_t* const stream) {
    return static_cast<uint64_t>(stream->len) * 8 + stream->cur_bit;
}

void uvg_bitstream_put_ue(bitstream_t* stream, uint32_t code_num) {
    const unsigned code_num_log2 = uvg_math_floor_log2(code_num + 1);
    const unsigned prefix = 1U << code_num_log2;
    const unsigned suffix = code_num + 1 - prefix;
    const unsigned num_bits = code_num_log2 * 2 + 1;
    const unsigned value = prefix | suffix;
    uvg_bitstream_put(stream, value, num_bits);
}

size_t uvg_calculate_ue_len(uint32_t number) {
    const unsigned code_num_log2 = uvg_math_floor_log2(number + 1);
    return code_num_log2 * 2 + 1;
}

void uvg_bitstream_add_rbsp_trailing_bits(bitstream_t* const stream) {
    uvg_bitstream_put(stream, 1, 1);
    if ((stream->cur_bit & 7U) != 0) {
        uvg_bitstream_put(stream, 0, 8 - (stream->cur_bit & 7U));
    }
}

void uvg_bitstream_align(bitstream_t* const stream) {
    if ((stream->cur_bit & 7U) != 0) {
        uvg_bitstream_add_rbsp_trailing_bits(stream);
    }
}

void uvg_bitstream_move(bitstream_t* const dst, bitstream_t* const src) {
    assert(dst->cur_bit == 0);
    if (src->len > 0) {
        if (dst->first == nullptr) {
            dst->first = src->first;
            dst->last = src->last;
            dst->len = src->len;
        } else {
            dst->last->next = src->first;
            dst->last = src->last;
            dst->len += src->len;
        }
    }
    dst->data = src->data;
    dst->cur_bit = src->cur_bit;
    src->first = src->last = nullptr;
    uvg_bitstream_clear(src);
}

void uvg_bitstream_copy_bytes(bitstream_t* const stream, const uint8_t* bytes, uint32_t len) {
    assert(stream->cur_bit == 0);
    if (stream->last == nullptr) {
        uvg_data_chunk* new_chunk = uvg_bitstream_alloc_chunk();
        assert(new_chunk);
        stream->first = stream->last = new_chunk;
    }
    uint32_t ptr = 0;
    uint32_t data_left = len;
    while (ptr < len) {
        const uint32_t space_left_in_chunk = UVG_DATA_CHUNK_SIZE - stream->last->len;
        if (data_left < space_left_in_chunk) {
            memcpy(&stream->last->data[stream->last->len], &bytes[ptr], data_left);
            ptr += data_left;
            stream->last->len += data_left;
            stream->len += data_left;
            data_left -= len - ptr;
        } else {
            memcpy(&stream->last->data[stream->last->len], &bytes[ptr], space_left_in_chunk);
            ptr += space_left_in_chunk;
            stream->last->len += space_left_in_chunk;
            stream->len += space_left_in_chunk;
            data_left -= space_left_in_chunk;
            if (stream->last == nullptr || stream->last->len == UVG_DATA_CHUNK_SIZE) {
                uvg_data_chunk* new_chunk = uvg_bitstream_alloc_chunk();
                assert(new_chunk);
                if (stream->first == nullptr) {
                    stream->first = new_chunk;
                }
                if (stream->last != nullptr) {
                    stream->last->next = new_chunk;
                }
                stream->last = new_chunk;
            }
        }
    }
}

uint32_t uvg_bitstream_peek_last_byte(bitstream_t* const stream) { return stream->data; }

}  // namespace uvgv3cbitstream
