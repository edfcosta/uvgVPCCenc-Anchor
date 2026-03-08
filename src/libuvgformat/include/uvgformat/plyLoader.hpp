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

/// \file PLY file loader for uvgformat.

#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "uvgFrame.hpp"

namespace uvgformat {

/// @brief Load a point cloud frame from a PLY file.
///
/// Reads the vertex geometry (x, y, z as uint16_t) and, if present, the RGB
/// color attribute (r, g, b as uint8_t). Points whose coordinates exceed
/// 2^geoBitDepthInput-1 are filtered out and a warning is logged.
///
/// @param filePath       Absolute or relative path to the .ply file.
/// @param frameNumber    Sequence number stored in the returned uvgFrame.
/// @param geoBitDepthInput  Bit depth of input geometry coordinates (e.g. 10 for vox10).
/// @return               Shared pointer to the populated uvgFrame.
/// @throws std::runtime_error if the file cannot be opened or is malformed.
std::shared_ptr<uvgFrame> loadPly(const std::string& filePath, size_t frameNumber, size_t geoBitDepthInput);

}  // namespace uvgformat
