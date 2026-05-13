/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 *
 * Copyright (c) 2024-present, Tampere University, ITU/ISO/IEC, project contributors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Tampere University, ITU/ISO/IEC nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * INCLUDING NEGLIGENCE OR OTHERWISE ARISING IN ANY WAY OUT OF THE USE OF THIS
 ****************************************************************************/

#include "adaptation/voxelization.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include "uvgutils/robin_hood.h"

#include "uvgutils/log.hpp"
#include "utils/parameters.hpp"

namespace uvgformat {

namespace {

// Pack 3 uint16_t coordinates into a uint64_t hash key (valid for geoPrecision <= 16).
inline uint64_t packKey(uint16_t x, uint16_t y, uint16_t z) {
    return (static_cast<uint64_t>(x) << 32) | (static_cast<uint64_t>(y) << 16) | static_cast<uint64_t>(z);
}

inline void unpackKey(uint64_t key, uint16_t& x, uint16_t& y, uint16_t& z) {
    x = static_cast<uint16_t>((key >> 32) & 0xFFFF);
    y = static_cast<uint16_t>((key >> 16) & 0xFFFF);
    z = static_cast<uint16_t>(key & 0xFFFF);
}

struct AttrAccum {
    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    uint32_t count = 0;
};

}  // anonymous namespace

uvgFramePayload voxelize(const std::vector<std::array<double, 3>>& rawGeo,
                         const std::vector<uvgutils::VectorN<uint8_t, 3>>* rawAttr) {
    const size_t geoPrecision = p_->geoPrecisionInput;
    const uint16_t maxCoord = static_cast<uint16_t>((1u << geoPrecision) - 1u);
    const size_t n = rawGeo.size();

    // Resolve bounds: NaN parameters mean compute the bounding box from the data.
    double minX = p_->voxelMinX;
    double minY = p_->voxelMinY;
    double minZ = p_->voxelMinZ;
    double maxX = p_->voxelMaxX;
    double maxY = p_->voxelMaxY;
    double maxZ = p_->voxelMaxZ;

    const bool needBbox = std::isnan(minX) || std::isnan(minY) || std::isnan(minZ) ||
                          std::isnan(maxX) || std::isnan(maxY) || std::isnan(maxZ);

    if (needBbox && n > 0) {
        double autoMinX = rawGeo[0][0], autoMinY = rawGeo[0][1], autoMinZ = rawGeo[0][2];
        double autoMaxX = rawGeo[0][0], autoMaxY = rawGeo[0][1], autoMaxZ = rawGeo[0][2];
        for (const auto& pt : rawGeo) {
            autoMinX = std::min(autoMinX, pt[0]);
            autoMinY = std::min(autoMinY, pt[1]);
            autoMinZ = std::min(autoMinZ, pt[2]);
            autoMaxX = std::max(autoMaxX, pt[0]);
            autoMaxY = std::max(autoMaxY, pt[1]);
            autoMaxZ = std::max(autoMaxZ, pt[2]);
        }
        if (std::isnan(minX)) minX = autoMinX;
        if (std::isnan(minY)) minY = autoMinY;
        if (std::isnan(minZ)) minZ = autoMinZ;
        if (std::isnan(maxX)) maxX = autoMaxX;
        if (std::isnan(maxY)) maxY = autoMaxY;
        if (std::isnan(maxZ)) maxZ = autoMaxZ;

        uvgutils::Logger::log<uvgutils::LogLevel::WARNING>(
            "UVGFORMAT", "Voxelization: Bounding box computed from data because some of voxelMinX/Y/Z or voxelMaxX/Y/Z were not set. Consistency across frames may be affected.\n");
        uvgutils::Logger::log<uvgutils::LogLevel::DEBUG>(
            "UVGFORMAT", "Voxelization bounding box: [" + std::to_string(minX) + ", " + std::to_string(maxX) + "] x [" +
                             std::to_string(minY) + ", " + std::to_string(maxY) + "] x [" + std::to_string(minZ) + ", " +
                             std::to_string(maxZ) + "]\n");
    }

    // Scale factors: map [min, max] -> [0, maxCoord].
    // A zero range maps all points to voxel 0 on that axis.
    const double rangeX = maxX - minX;
    const double rangeY = maxY - minY;
    const double rangeZ = maxZ - minZ;

    double scaleX{};
    double scaleY{};
    double scaleZ{};
    int64_t offsetX = 0;
    int64_t offsetY = 0;
    int64_t offsetZ = 0;
    if (p_->keepGeoRatio) {
        // Single scale derived from the largest range so aspect ratio is preserved.
        const double maxRange = std::max({rangeX, rangeY, rangeZ});
        const double scale = (maxRange > 0.0) ? (static_cast<double>(maxCoord) / maxRange) : 0.0;
        scaleX = scaleY = scaleZ = scale;
        // Center each axis in the grid: shorter axes don't fill the full range, so
        // shift them so their center lands at maxCoord/2.
        offsetX = std::llround((static_cast<double>(maxCoord) - rangeX * scale) / 2.0);
        offsetY = std::llround((static_cast<double>(maxCoord) - rangeY * scale) / 2.0);
        offsetZ = std::llround((static_cast<double>(maxCoord) - rangeZ * scale) / 2.0);
    } else {
        scaleX = (rangeX > 0.0) ? (static_cast<double>(maxCoord) / rangeX) : 0.0;
        scaleY = (rangeY > 0.0) ? (static_cast<double>(maxCoord) / rangeY) : 0.0;
        scaleZ = (rangeZ > 0.0) ? (static_cast<double>(maxCoord) / rangeZ) : 0.0;
        // Each axis fills [0, maxCoord] independently — already centered, no shift needed.
    }

    size_t skipped = 0;

    if (rawAttr != nullptr) {
        // GeometryRgb: accumulate attribute values per voxel, average at the end.
        robin_hood::unordered_map<uint64_t, AttrAccum> accumMap;
        accumMap.reserve(n);
        std::vector<uint64_t> insertionOrder;
        insertionOrder.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            const int64_t vx = static_cast<int64_t>(std::floor((rawGeo[i][0] - minX) * scaleX)) + offsetX;
            const int64_t vy = static_cast<int64_t>(std::floor((rawGeo[i][1] - minY) * scaleY)) + offsetY;
            const int64_t vz = static_cast<int64_t>(std::floor((rawGeo[i][2] - minZ) * scaleZ)) + offsetZ;

            if (vx < 0 || vy < 0 || vz < 0 || vx > maxCoord || vy > maxCoord || vz > maxCoord) {
                ++skipped;
                continue;
            }

            const uint64_t key =
                packKey(static_cast<uint16_t>(vx), static_cast<uint16_t>(vy), static_cast<uint16_t>(vz));
            auto [it, inserted] = accumMap.emplace(key, AttrAccum{});
            if (inserted) {
                insertionOrder.push_back(key);
            }
            auto& acc = it->second;
            acc.r += (*rawAttr)[i][0];
            acc.g += (*rawAttr)[i][1];
            acc.b += (*rawAttr)[i][2];
            ++acc.count;
        }

        if (skipped > 0) {
            uvgutils::Logger::log<uvgutils::LogLevel::WARNING>(
                "UVGFORMAT",
                std::to_string(skipped) + " point(s) fell outside the voxel grid after centering and were discarded.\n");
        }

        GeometryRgb result;
        result.geometry.reserve(insertionOrder.size());
        result.attribute.reserve(insertionOrder.size());
        for (const uint64_t key : insertionOrder) {
            uint16_t x{}, y{}, z{};
            unpackKey(key, x, y, z);
            result.geometry.push_back({x, y, z});
            const auto& acc = accumMap.at(key);
            result.attribute.push_back({static_cast<uint8_t>(acc.r / acc.count),
                                        static_cast<uint8_t>(acc.g / acc.count),
                                        static_cast<uint8_t>(acc.b / acc.count)});
        }
        return result;

    } else {
        // GeometryOnly: deduplication only, first occurrence wins.
        robin_hood::unordered_set<uint64_t> seenKeys;
        seenKeys.reserve(n);
        std::vector<uint64_t> insertionOrder;
        insertionOrder.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            const int64_t vx = static_cast<int64_t>(std::floor((rawGeo[i][0] - minX) * scaleX)) + offsetX;
            const int64_t vy = static_cast<int64_t>(std::floor((rawGeo[i][1] - minY) * scaleY)) + offsetY;
            const int64_t vz = static_cast<int64_t>(std::floor((rawGeo[i][2] - minZ) * scaleZ)) + offsetZ;

            if (vx < 0 || vy < 0 || vz < 0 || vx > maxCoord || vy > maxCoord || vz > maxCoord) {
                ++skipped;
                continue;
            }

            const uint64_t key =
                packKey(static_cast<uint16_t>(vx), static_cast<uint16_t>(vy), static_cast<uint16_t>(vz));
            if (seenKeys.insert(key).second) {
                insertionOrder.push_back(key);
            }
        }

        if (skipped > 0) {
            uvgutils::Logger::log<uvgutils::LogLevel::WARNING>(
                "UVGFORMAT",
                std::to_string(skipped) + " point(s) fell outside the voxel grid after centering and were discarded.\n");
        }

        GeometryOnly result;
        result.geometry.reserve(insertionOrder.size());
        for (const uint64_t key : insertionOrder) {
            uint16_t x{}, y{}, z{};
            unpackKey(key, x, y, z);
            result.geometry.push_back({x, y, z});
        }
        return result;
    }
}

}  // namespace uvgformat
