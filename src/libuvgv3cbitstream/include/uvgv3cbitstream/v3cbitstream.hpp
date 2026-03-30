/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 *
 * Copyright (c) 2024-present, Tampere University, ITU/ISO/IEC, project contributors
 * All rights reserved.
 ****************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <semaphore>
#include <vector>

#include "atlas.hpp"
#include "vps.hpp"

namespace uvgv3cbitstream {

static inline int floorLog2(uint32_t x) {
    if (x == 0) {
        return -1;
    }
#ifdef __GNUC__
    return 31 - __builtin_clz(x);
#else
    int result = 0;
    if (x & 0xffff0000) {
        x >>= 16;
        result += 16;
    }
    if (x & 0xff00) {
        x >>= 8;
        result += 8;
    }
    if (x & 0xf0) {
        x >>= 4;
        result += 4;
    }
    if (x & 0xc) {
        x >>= 2;
        result += 2;
    }
    if (x & 0x2) {
        result += 1;
    }
    return result;
#endif
}

#define UVG_DATA_CHUNK_SIZE 4096

/// @brief One complete V3C unit, including the V3C unit header.
struct v3c_unit {
    VUT type = uvgv3cbitstream::V3C_VPS;
    size_t len = 0;                // Length of data in buffer
    std::unique_ptr<char[]> data;  // Actual data (char type can be used to describe a byte. No need for uint8_t or unsigned char types.)

    v3c_unit() = default;
    v3c_unit(VUT type, size_t len, std::unique_ptr<char[]> data) : type(type), len(len), data(std::move(data)) {}
};

/// @brief One GOF-worth of ordered V3C units emitted by the encoder.
struct v3c_unit_batch {
    std::vector<v3c_unit> v3c_units = {};
};

struct v3c_unit_stream {
    std::queue<v3c_unit_batch> v3c_unit_batches = {};
    std::counting_semaphore<> available_chunks{0};
    std::mutex io_mutex;  // Locks production and consumption in the v3c_unit_batches queue
};

typedef struct uvg_data_chunk {
    uint8_t data[UVG_DATA_CHUNK_SIZE];
    uint32_t len;
    struct uvg_data_chunk* next;
} uvg_data_chunk;

typedef struct bitstream_t {
    uint32_t len;
    uvg_data_chunk* first;
    uvg_data_chunk* last;
    uint8_t data;
    uint8_t cur_bit;
} bitstream_t;

struct nal_info {
    size_t location = 0;
    size_t size = 0;
};

struct V3cGof {
    size_t gof_id = 0;
    size_t n_frames = 0;
    std::unique_ptr<Vps> vps = nullptr;
    std::unique_ptr<AtlasContext> atlas = nullptr;
    std::unique_ptr<std::vector<uint8_t>> ovd = nullptr;
    std::unique_ptr<std::vector<uint8_t>> gvd = nullptr;
    std::unique_ptr<std::vector<uint8_t>> avd = nullptr;
};

struct SerializedUnit {
    VUT type = V3C_VPS;
    size_t len = 0;
    std::unique_ptr<char[]> data;
};

static inline int ceilLog2(uint32_t x) { return (x == 0) ? -1 : floorLog2(x - 1) + 1; }

void uvg_bitstream_init(bitstream_t* stream);
uvg_data_chunk* uvg_bitstream_alloc_chunk();
void uvg_bitstream_free_chunks(uvg_data_chunk* chunk);
void uvg_bitstream_writebyte(bitstream_t* stream, uint8_t byte);
void uvg_bitstream_put(bitstream_t* stream, uint32_t data, uint8_t bits);
uvg_data_chunk* uvg_bitstream_take_chunks(bitstream_t* stream);
void uvg_bitstream_finalize(bitstream_t* stream);
void uvg_bitstream_clear(bitstream_t* stream);
uint64_t uvg_bitstream_tell(const bitstream_t* stream);
void uvg_bitstream_put_ue(bitstream_t* stream, uint32_t code_num);
size_t uvg_calculate_ue_len(uint32_t number);
void uvg_bitstream_add_rbsp_trailing_bits(bitstream_t* stream);
void uvg_bitstream_align(bitstream_t* stream);
void uvg_bitstream_move(bitstream_t* dst, bitstream_t* src);
void uvg_bitstream_copy_bytes(bitstream_t* stream, const uint8_t* bytes, uint32_t len);
uint32_t uvg_bitstream_peek_last_byte(bitstream_t* stream);

void prepareAtlasContext(AtlasContext& atlas);
bool writeVps(bitstream_t* stream, const Vps& vps);
void writeAtlasSubBitstream(bitstream_t* stream, const AtlasContext& atlas);
void writeAtlasParameterSetNals(bitstream_t* stream, const AtlasContext& atlas);
void writeAtlasNal(bitstream_t* stream, const AtlasContext& atlas, size_t index);
void writeAtlasEob(bitstream_t* stream, const AtlasContext& atlas);
std::vector<SerializedUnit> writeV3cUnits(const V3cGof& gof);
std::vector<SerializedUnit> writeV3cLdUnits(const V3cGof& gof, const std::vector<nal_info>& ovd_nals, const std::vector<nal_info>& gvd_nals,
                                            const std::vector<nal_info>& avd_nals, bool double_layer);

}  // namespace uvgv3cbitstream
