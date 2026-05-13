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

/// \file Generic parameter type system, validation, and string-to-type conversion.

#include "uvgutils/parameterManager.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <exception>
#include <limits>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include "uvgutils/log.hpp"

namespace uvgutils {

namespace {

inline int toInt(const std::string& paramValue, const std::string& paramName) {
    try {
        size_t pos = 0;
        const int value = std::stoi(paramValue, &pos);
        if (pos != paramValue.length()) {
            throw std::invalid_argument("");
        }
        return value;
    } catch (const std::exception& e) {
        throw std::runtime_error("During the parsing of the uvgVPCC library command, an error occured: " + std::string(e.what()) +
                                 "\nThe value assign to '" + paramName + "' is: '" + paramValue +
                                 "'\nThis value was not converted into an int.");
    }
}

inline size_t toUInt(const std::string& paramValue, const std::string& paramName) {
    try {
        if (paramValue[0] == '-') {
            throw std::runtime_error("");
        }
        size_t pos = 0;
        const size_t value = static_cast<size_t>(std::stoi(paramValue, &pos));
        if (pos != paramValue.length()) {
            throw std::invalid_argument("");
        }
        // TODO(lf): check the overflow during int and size_t conversion
        return value;
    } catch (const std::exception& e) {
        throw std::runtime_error("During the parsing of the uvgVPCC library command, an error occured: " + std::string(e.what()) +
                                 "\nThe value assign to '" + paramName + "' is: '" + paramValue +
                                 "'\nThis value was not converted into an unsigned int (size_t).");
    }
}

inline float toFloat(const std::string& paramValue, const std::string& paramName) {
    try {
        size_t pos = 0;
        const float value = std::stof(paramValue, &pos);
        if (pos != paramValue.length()) {
            throw std::invalid_argument("");
        }
        return value;
    } catch (const std::exception& e) {
        throw std::runtime_error("During the parsing of the uvgVPCC library command, an error occured: " + std::string(e.what()) +
                                 "\nThe value assign to '" + paramName + "' is: '" + paramValue +
                                 "'\nThis value was not converted into a float.");
    }
}

inline double toDouble(const std::string& paramValue, const std::string& paramName) {
    try {
        size_t pos = 0;
        const double value = std::stod(paramValue, &pos);
        if (pos != paramValue.length()) {
            throw std::invalid_argument("");
        }
        // TODO(lf): check the overflow during int and size_t conversion
        return value;
    } catch (const std::exception& e) {
        throw std::runtime_error("During the parsing of the uvgVPCC library command, an error occured: " + std::string(e.what()) +
                                 "\nThe value assign to '" + paramName + "' is: '" + paramValue +
                                 "'\nThis value was not converted into a double.");
    }
}

inline bool toBool(const std::string& paramValue, const std::string& paramName) {
    if (paramValue == "true" || paramValue == "True" || paramValue == "1") {
        return true;
    }
    if (paramValue == "false" || paramValue == "False" || paramValue == "0") {
        return false;
    }
    throw std::runtime_error("During the parsing of the uvgVPCC library command, an error occured.\nThe value assign to '" + paramName +
                             "' is: '" + paramValue +
                             "'\nThis value was not converted into a boolean. Only those values are accepted: [true,false,1,0]");
}

size_t levenshteinDistance(const std::string& a, const std::string& b) {
    const size_t m = a.size();
    const size_t n = b.size();
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1));
    for (size_t i = 0; i <= m; ++i) {
        dp[i][0] = i;
    }
    for (size_t j = 0; j <= n; ++j) {
        dp[0][j] = j;
    }
    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            const int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,        // Deletion
                dp[i][j - 1] + 1,        // Insertion
                dp[i - 1][j - 1] + cost  // Substitution
            });
        }
    }
    return dp[m][n];
}

std::string suggestClosestString(const ParameterMap& parameterMap, const std::string& inputStr) {
    size_t minDistance = std::numeric_limits<size_t>::max();
    std::string closestString;
    for (const auto& option : parameterMap) {
        const size_t distance = levenshteinDistance(inputStr, option.first);
        if (distance < minDistance) {
            minDistance = distance;
            closestString = option.first;
        }
    }
    return closestString;
}

}  // anonymous namespace

void setParameterValue(ParameterMap& parameterMap, const std::string& parameterName, const std::string& parameterValue,
                       bool fromPreset) {
    uvgutils::Logger::log<uvgutils::LogLevel::DEBUG>("API", "Set parameter value: " + parameterName + " -> " + parameterValue + "\n");

    if (!parameterMap.contains(parameterName)) {
        throw std::invalid_argument(std::string(fromPreset ? "[PRESET] " : "") + "The parameter '" + parameterName +
                                    "' is not a valid parameter name. Did you mean '" +
                                    suggestClosestString(parameterMap, parameterName) + "'? (c.f. parameterMap)");
    }
    if (parameterValue.empty()) {
        throw std::invalid_argument("It seems an empty value is assigned to the parameter " + parameterName + ".");
    }
    ParameterInfo& paramInfo = parameterMap.find(parameterName)->second;
    if (!paramInfo.possibleValues.empty()) {
        const std::regex possibleValueRegex("^(" + std::regex_replace(paramInfo.possibleValues, std::regex(","), "|") + ")$");
        if (!std::regex_match(parameterValue, possibleValueRegex)) {
            throw std::invalid_argument("Invalid value for parameter '" + parameterName + "': '" + parameterValue +
                                        "'. Accepted values are: [" + paramInfo.possibleValues + "]");
        }
    }

    switch (paramInfo.type) {
        case INT:
            *static_cast<int*>(paramInfo.parameterPtr) = toInt(parameterValue, parameterName);
            break;
        case BOOL:
            *static_cast<bool*>(paramInfo.parameterPtr) = toBool(parameterValue, parameterName);
            break;
        case UINT:
            *static_cast<size_t*>(paramInfo.parameterPtr) = toUInt(parameterValue, parameterName);
            break;
        case FLOAT:
            *static_cast<float*>(paramInfo.parameterPtr) = toFloat(parameterValue, parameterName);
            break;
        case DOUBLE:
            *static_cast<double*>(paramInfo.parameterPtr) = toDouble(parameterValue, parameterName);
            break;
        case STRING:
            *static_cast<std::string*>(paramInfo.parameterPtr) = parameterValue;
            break;
        default:
            assert(false);
    }

    if (fromPreset) {
        paramInfo.inPreset = true;
    } else if (paramInfo.inPreset) {
        uvgutils::Logger::log<uvgutils::LogLevel::INFO>(
            "API", "The value assigned to parameter '" + parameterName + "' overwrite the preset value.\n");
    }
}

}  // namespace uvgutils
