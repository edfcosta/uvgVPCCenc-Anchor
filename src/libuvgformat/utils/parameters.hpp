/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 *
 * Copyright (c) 2024, Tampere University, ITU/ISO/IEC, project contributors
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

/// \file Library parameters related operations.

#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "uvgutils/parameterManager.hpp"
#include "uvgutils/utils.hpp"

namespace uvgformat {

struct Parameters {
    size_t geoPrecisionInput = 0;  // Must be set before initializeFormat().
    std::string logLevel = "INFO";

    // Voxelization adaptation
    bool enableVoxelization = false;
    bool keepGeoRatio = false;  // Use a single scale (largest range) so aspect ratio is kept.
    double voxelMin = std::numeric_limits<double>::quiet_NaN();  // NaN = compute from data, overrides per-axis if set
    double voxelMinX = std::numeric_limits<double>::quiet_NaN();  // NaN = compute from data
    double voxelMinY = std::numeric_limits<double>::quiet_NaN();
    double voxelMinZ = std::numeric_limits<double>::quiet_NaN();
    double voxelMax = std::numeric_limits<double>::quiet_NaN();  // NaN = compute from data, overrides per-axis if set
    double voxelMaxX = std::numeric_limits<double>::quiet_NaN();  // NaN = compute from data
    double voxelMaxY = std::numeric_limits<double>::quiet_NaN();
    double voxelMaxZ = std::numeric_limits<double>::quiet_NaN();
};

/// @brief Read-only view of the active parameters. nullptr until initializeFormat() is called.
extern const Parameters* p_; 

void initializeParameterMap(Parameters& param);
void setParameterValue(const std::string& parameterName, const std::string& parameterValue, const bool& fromPreset);

}  // namespace uvgformat