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

/// \file Format library parameters implementation.

#include "parameters.hpp"

#include <numeric>
#include <string>
#include <unordered_map>

#include "uvgutils/log.hpp"
#include "uvgutils/parameterManager.hpp"

namespace uvgformat {

namespace {

    uvgutils::ParameterMap parameterMap;

}  // anonymous namespace

void initializeParameterMap(Parameters& param) {
    using uvgutils::BOOL;
    using uvgutils::UINT;
    using uvgutils::STRING;
    using uvgutils::DOUBLE;
    const std::string logLevelPossibleValues =
        std::accumulate(std::next(std::begin(uvgutils::LogLevelStr)), std::end(uvgutils::LogLevelStr), uvgutils::LogLevelStr[0],
                        [](const std::string& a, const std::string& b) { return a + "," + b; });
    parameterMap = {
        {"geoPrecisionInput", {UINT,   "", &param.geoPrecisionInput}},
        {"logLevel",          {STRING, logLevelPossibleValues, &param.logLevel}},
        {"enableVoxelization", {BOOL,   "", &param.enableVoxelization}},
        {"keepGeoRatio",       {BOOL,   "", &param.keepGeoRatio}},
        {"voxelMin",           {DOUBLE, "", &param.voxelMin}},
        {"voxelMinX",          {DOUBLE, "", &param.voxelMinX}},
        {"voxelMinY",          {DOUBLE, "", &param.voxelMinY}},
        {"voxelMinZ",          {DOUBLE, "", &param.voxelMinZ}},
        {"voxelMax",           {DOUBLE, "", &param.voxelMax}},
        {"voxelMaxX",          {DOUBLE, "", &param.voxelMaxX}},
        {"voxelMaxY",          {DOUBLE, "", &param.voxelMaxY}},
        {"voxelMaxZ",          {DOUBLE, "", &param.voxelMaxZ}},
    };
}

void setParameterValue(const std::string& parameterName, const std::string& parameterValue, const bool& fromPreset) {
    // Handle uniform shorthand parameters (set all three axes at once)
    if (parameterName == "voxelMin") {
        uvgutils::setParameterValue(parameterMap, "voxelMinX", parameterValue, fromPreset);
        uvgutils::setParameterValue(parameterMap, "voxelMinY", parameterValue, fromPreset);
        uvgutils::setParameterValue(parameterMap, "voxelMinZ", parameterValue, fromPreset);
        return;
    }
    if (parameterName == "voxelMax") {
        uvgutils::setParameterValue(parameterMap, "voxelMaxX", parameterValue, fromPreset);
        uvgutils::setParameterValue(parameterMap, "voxelMaxY", parameterValue, fromPreset);
        uvgutils::setParameterValue(parameterMap, "voxelMaxZ", parameterValue, fromPreset);
        return;
    }
    uvgutils::setParameterValue(parameterMap, parameterName, parameterValue, fromPreset);
}

}  // namespace uvgformat
