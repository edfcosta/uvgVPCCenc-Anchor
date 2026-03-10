/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 *
 * Copyright (c) 2024-present, Tampere University, ITU/ISO/IEC, project contributors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of the Tampere University or ITU/ISO/IEC nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
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

/// \file Point cloud attribute payload types and the AttributeData variant.

#pragma once

#include <cstdint>
#include <stdexcept>
#include <variant>
#include <vector>

#include "uvgutils/utils.hpp"

namespace uvgformat {

/// Geometry-only point cloud (no color or normals).
struct GeometryOnly {
    std::vector<uvgutils::VectorN<uint16_t, 3>> geometry;
};

/// Point cloud with geometry (XYZ) and RGB color attribute.
/// This is the format required by the uvgVPCCenc encoder.
struct GeometryRgb {
    std::vector<uvgutils::VectorN<uint16_t, 3>> geometry;
    std::vector<uvgutils::VectorN<uint8_t, 3>> attribute;
};

/// Point cloud with geometry, RGB color and pre-computed normals (reserved for future use).
struct GeometryRgbNormals {
    std::vector<uvgutils::VectorN<uint16_t, 3>> geometry;
    std::vector<uvgutils::VectorN<uint8_t, 3>> attribute;
    std::vector<uvgutils::VectorN<float, 3>> normals;
};

/// Variant holding the active combination of point cloud attributes.
using uvgFramePayload = std::variant<GeometryOnly, GeometryRgb, GeometryRgbNormals>;

/// @brief Access the GeometryRgb alternative of an uvgFramePayload variant.
/// @throws std::bad_variant_access if the active alternative is not GeometryRgb.
inline GeometryRgb& getGeometryRgb(uvgFramePayload& data) { return std::get<GeometryRgb>(data); }

/// @brief Const overload.
inline const GeometryRgb& getGeometryRgb(const uvgFramePayload& data) { return std::get<GeometryRgb>(data); }

}  // namespace uvgformat
