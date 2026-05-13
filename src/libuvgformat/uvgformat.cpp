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

/// \file 

#include "utils/parameters.hpp"
#include "uvgformat/uvgformat.hpp"

#include <stdexcept>

#include "uvgutils/log.hpp"

namespace uvgformat {

namespace {

Parameters param;
// Map storing all parameters intending to change the state of the encoder from outside of the library.
std::unordered_map<std::string, std::string> apiInputParameters;
bool errorInAPI = false;
bool initializationDone = false;


void setLogParameters() {
    auto logLevelIt = apiInputParameters.find("logLevel");
    if (logLevelIt != apiInputParameters.end()) {  // logLevel has been defined by the application
        setParameterValue("logLevel", logLevelIt->second, false);
        uvgutils::Logger::log<uvgutils::LogLevel::INFO>("API", "The logLevel is set to '" + p_->logLevel + "'.\n");
    } else {  // logLevel default value
        setParameterValue("logLevel", uvgutils::LogLevelStr[static_cast<size_t>(uvgutils::logLevelDefaultValue)], false);
        uvgutils::Logger::log<uvgutils::LogLevel::INFO>("API", "The logLevel is set by default to '" + p_->logLevel + "'.\n");
    }
    uvgutils::Logger::setLogLevel(static_cast<uvgutils::LogLevel>(std::distance(
        std::begin(uvgutils::LogLevelStr), std::find(std::begin(uvgutils::LogLevelStr), std::end(uvgutils::LogLevelStr), p_->logLevel))));
}

void setGeoPrecisionInput() {
    auto geoPrecisionInputIt = apiInputParameters.find("geoPrecisionInput");
    if (geoPrecisionInputIt == apiInputParameters.end()) {
        throw std::runtime_error("The parameter 'geoPrecisionInput' has to be defined.");
    }

    // geoPrecisionInput has been defined by the application
    setParameterValue("geoPrecisionInput", geoPrecisionInputIt->second, false);
    uvgutils::Logger::log<uvgutils::LogLevel::INFO>("UVGFORMAT", "The geoPrecisionInput is set to '" + std::to_string(p_->geoPrecisionInput) + "'.\n");

}

void parseUvgformatParameters() {
    // Special parameters need to be handle first
    setLogParameters();
    setGeoPrecisionInput();

    // Now that the preset is applied, all other parameters set by the application can overwrite the preset values.
    for (const auto& paramPair : apiInputParameters) {
        if (paramPair.first == "geoPrecisionInput" || paramPair.first == "logLevel") {
            // Those parameters have been handled at the top of this function
            continue;
        }
        setParameterValue(paramPair.first, paramPair.second, false);
    }
}

}  // anonymous namespace

const Parameters* p_ = &param;


/// @brief Create the context of the uvgVPCCenc encoder. Parse the input parameters and verify if the given configuration is valid. Initialize
/// static parameters and function pointers.
void API::initializeFormat() {
    uvgutils::Logger::log<uvgutils::LogLevel::TRACE>("API", "Initialize the encoder.\n");
    initializeParameterMap(param);
    parseUvgformatParameters();
    initializationDone = true;
}

/// @brief The only way to modify the exposed uvgformat parameters is by calling this function.
/// @param parameterName Name of the parameter. All exposed parameters are listed in the object parameterMap defined in
/// uvgformat/utils/parameters.cpp
/// @param parameterValue The value of the parameter written as a string.
void API::setParameter(const std::string& parameterName, const std::string& parameterValue) {
    if (initializationDone) {
        uvgutils::Logger::log<uvgutils::LogLevel::FATAL>(
            "API", "The API function 'setParameter' can't be called after the API function 'initializeFormat'.\n");
        throw std::runtime_error("");
    }
    if (apiInputParameters.find(parameterName) != apiInputParameters.end()) {
        uvgutils::Logger::log<uvgutils::LogLevel::ERROR>("API", "The parameter '" + parameterName +
                                                                    "' has already been set. The value used is: '" +
                                                                    apiInputParameters.at(parameterName) + "'.\n");
        errorInAPI = true;
    }
    apiInputParameters.emplace(parameterName, parameterValue);
}

}  // namespace uvgformat