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

/// \file Voxelization adaptation: float-to-integer quantization with deduplication
/// and attribute averaging for overlapping points.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "uvgformat/uvgFramePayload.hpp"
#include "uvgutils/utils.hpp"

namespace uvgformat {

/// @brief Voxelize a raw point cloud using the active library parameters (p_).
///
/// Reads voxelMinX/Y/Z, voxelMaxX/Y/Z (NaN = compute bounding box from data),
/// voxelOffsetX/Y/Z, and geoPrecisionInput from p_. Maps each input point into
/// the [0, 2^geoPrecisionInput - 1]^3 integer grid, deduplicates, and for colored
/// clouds averages the attributes of points that fall into the same voxel.
/// Points that fall outside the grid after offset are discarded with a WARNING log.
/// Insertion order is preserved for deterministic output.
///
/// @param rawGeo   Input geometry in original floating-point coordinates.
/// @param rawAttr  Per-point RGB attribute, or nullptr for geometry-only output.
/// @return GeometryOnly or GeometryRgb payload with integer voxel coordinates.
uvgFramePayload voxelize(const std::vector<std::array<double, 3>>& rawGeo,
                         const std::vector<uvgutils::VectorN<uint8_t, 3>>* rawAttr);

}  // namespace uvgformat
