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

#include "uvgformat/uvgformat.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "adaptation/voxelization.hpp"
#include "io/miniply.h"
#include "uvgformat/uvgFramePayload.hpp"
#include "uvgformat/uvgFrame.hpp"
#include "uvgutils/log.hpp"
#include "uvgutils/utils.hpp"
#include "utils/parameters.hpp"

namespace uvgformat {

std::shared_ptr<uvgFrame> API::loadPly(const std::string& filePath, const size_t frameNumber) {
    if (p_ == nullptr) {
        throw std::runtime_error("uvgformat::loadPly called before uvgformat::initializeFormat()");
    }
    uvgutils::Logger::log<uvgutils::LogLevel::TRACE>("UVGFORMAT",
                                                     "Loading frame " + std::to_string(frameNumber) + " from " + filePath + "\n");

    if (!std::filesystem::is_regular_file(filePath)) {
        throw std::runtime_error("\nThis path does not exist: " + filePath);
    }

    miniply::PLYReader reader(filePath.c_str());
    if (!reader.valid()) {
        throw std::runtime_error("\nThe miniply reader failed to open " + filePath);
    }

    bool vertexElementFound = false;
    while (reader.has_element()) {
        if (reader.element_is(miniply::kPLYVertexElement)) {
            vertexElementFound = true;
            break;
        }
        reader.next_element();
    }

    if (!vertexElementFound) {
        throw std::runtime_error("miniply : No vertex element (miniply::kPLYVertexElement) was found in this file : " + filePath);
    }

    if (!reader.load_element()) {
        throw std::runtime_error("miniply : Vertex element did not load correctly (file: " + filePath + ")");
    }

    std::array<uint32_t, 3> indicesPos{};
    if (!reader.find_pos(indicesPos.data())) {
        throw std::runtime_error(
            "miniply : Position properties (x,y,z) were not located in the vertex element (file: " + filePath + ")");
    }

    std::array<uint32_t, 3> indicesCol{};
    const bool hasColor = reader.find_color(indicesCol.data());

    const size_t vertexCount = reader.element()->count;

    auto result = std::make_shared<uvgFrame>();
    result->frameNumber = frameNumber;
    result->sourcePath = filePath;

    // Read geometry as double so any coordinate type stored in the PLY is handled
    // uniformly. The adaptation step (voxelization) or the direct cast below will
    // convert to uint16_t.
    std::vector<std::array<double, 3>> rawGeo(vertexCount);
    reader.extract_properties(indicesPos.data(), 3, miniply::PLYPropertyType::Double, rawGeo.data());

    std::vector<uvgutils::VectorN<uint8_t, 3>> rawAttr;
    if (hasColor) {
        rawAttr.resize(vertexCount);
        reader.extract_properties(indicesCol.data(), 3, miniply::PLYPropertyType::UChar, rawAttr.data());
    }

    uvgutils::Logger::log<uvgutils::LogLevel::DEBUG>(
        "UVGFORMAT", "Frame " + std::to_string(frameNumber) + " : path: " + filePath + "\n\tpointsGeometry size: " +
                         std::to_string(vertexCount) + (hasColor ? ("\n\tpointsAttribute size: " + std::to_string(vertexCount)) : "") +
                         "\n");

    if (p_->enableVoxelization) {
        result->payload = voxelize(rawGeo, hasColor ? &rawAttr : nullptr);
        return result;
    }

    // --- Non-voxelized path: direct double -> uint16_t cast ---
    // Round to nearest integer; rely on the existing compliance filter below to
    // remove any point that exceeds the geoPrecisionInput bit-depth constraint.
    const size_t geoPrecisionInput = p_->geoPrecisionInput;

    if (hasColor) {
        GeometryRgb payload;
        payload.geometry.resize(vertexCount);
        payload.attribute = std::move(rawAttr);
        for (size_t i = 0; i < vertexCount; ++i) {
            payload.geometry[i][0] = static_cast<uint16_t>(std::lround(rawGeo[i][0]));
            payload.geometry[i][1] = static_cast<uint16_t>(std::lround(rawGeo[i][1]));
            payload.geometry[i][2] = static_cast<uint16_t>(std::lround(rawGeo[i][2]));
        }
        result->payload = std::move(payload);
    } else {
        GeometryOnly payload;
        payload.geometry.resize(vertexCount);
        for (size_t i = 0; i < vertexCount; ++i) {
            payload.geometry[i][0] = static_cast<uint16_t>(std::lround(rawGeo[i][0]));
            payload.geometry[i][1] = static_cast<uint16_t>(std::lround(rawGeo[i][1]));
            payload.geometry[i][2] = static_cast<uint16_t>(std::lround(rawGeo[i][2]));
        }
        result->payload = std::move(payload);
    }

    // Filter points that violate the bit-depth constraint.
    auto& geo = std::visit([](auto& p) -> std::vector<uvgutils::VectorN<uint16_t, 3>>& { return p.geometry; }, result->payload);

    const bool isCompliant = !std::any_of(geo.begin(), geo.end(), [geoPrecisionInput](const uvgutils::VectorN<uint16_t, 3>& point) {
        return (point[0] >> geoPrecisionInput) | (point[1] >> geoPrecisionInput) | (point[2] >> geoPrecisionInput);
    });

    if (!isCompliant) {
        uvgutils::Logger::log<uvgutils::LogLevel::ERROR>(
            "UVGFORMAT",
            "Frame " + std::to_string(frameNumber) + " from " + filePath +
                " contains at least one point which does not respect the input voxel size (geoPrecisionInput = " +
                std::to_string(geoPrecisionInput) + "). Maximum value is 2^" + std::to_string(geoPrecisionInput) +
                "-1. All faulty points will not be processed.\n");

        if (hasColor) {
            auto& rgbPayload = std::get<GeometryRgb>(result->payload);
            std::vector<uvgutils::VectorN<uint16_t, 3>> geoTmp;
            std::vector<uvgutils::VectorN<uint8_t, 3>> attrTmp;
            geoTmp.reserve(rgbPayload.geometry.size());
            attrTmp.reserve(rgbPayload.geometry.size());
            for (size_t i = 0; i < rgbPayload.geometry.size(); ++i) {
                const auto& pt = rgbPayload.geometry[i];
                if ((pt[0] >> geoPrecisionInput) | (pt[1] >> geoPrecisionInput) | (pt[2] >> geoPrecisionInput)) continue;
                geoTmp.emplace_back(pt);
                attrTmp.emplace_back(rgbPayload.attribute[i]);
            }
            rgbPayload.geometry.swap(geoTmp);
            rgbPayload.attribute.swap(attrTmp);
        } else {
            auto& geoPayload = std::get<GeometryOnly>(result->payload);
            std::vector<uvgutils::VectorN<uint16_t, 3>> geoTmp;
            geoTmp.reserve(geoPayload.geometry.size());
            for (const auto& pt : geoPayload.geometry) {
                if ((pt[0] >> geoPrecisionInput) | (pt[1] >> geoPrecisionInput) | (pt[2] >> geoPrecisionInput)) continue;
                geoTmp.emplace_back(pt);
            }
            geoPayload.geometry.swap(geoTmp);
        }
    }

    return result;
}

}  // namespace uvgformat
