/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 *
 * Copyright (c) 2024, Tampere University, ITU/ISO/IEC, project contributors
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

/// \file Generic parameter type system, validation, and string-to-type conversion.

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace uvgutils {

enum ParameterType { BOOL, INT, UINT, STRING, FLOAT, DOUBLE };

struct ParameterInfo {
    ParameterType type;
    std::string possibleValues;
    void* parameterPtr;
    bool inPreset = false;

    ParameterInfo(const ParameterType& type, const std::string& possibleValues, bool* parameterPtr)
        : type(type), possibleValues(possibleValues), parameterPtr((void*)parameterPtr) {
        if (type != BOOL) {
            throw std::runtime_error(
                "During the initialization of the library parameter maps, a type mismatch has been found. Apparently, the given "
                "parameterType is: '" +
                std::to_string(type) +
                "' while the type of the parameter variable is BOOL (0). The corresponding variable name is not known, but here are its "
                "possible values :'" +
                possibleValues + "'. If you recently added a new parameter in the parameter map, the given type is probably wrong.");
        }
    }
    ParameterInfo(const ParameterType& type, const std::string& possibleValues, int* parameterPtr)
        : type(type), possibleValues(possibleValues), parameterPtr((void*)parameterPtr) {
        if (type != INT) {
            throw std::runtime_error(
                "During the initialization of the library parameter maps, a type mismatch has been found. Apparently, the given "
                "parameterType is: '" +
                std::to_string(type) +
                "' while the type of the parameter variable is INT (1). The corresponding variable name is not known, but here are its "
                "possible values :'" +
                possibleValues + "'. If you recently added a new parameter in the parameter map, the given type is probably wrong.");
        }
    }
    ParameterInfo(const ParameterType& type, const std::string& possibleValues, size_t* parameterPtr)
        : type(type), possibleValues(possibleValues), parameterPtr((void*)parameterPtr) {
        if (type != UINT) {
            throw std::runtime_error(
                "During the initialization of the library parameter maps, a type mismatch has been found. Apparently, the given "
                "parameterType is: '" +
                std::to_string(type) +
                "' while the type of the parameter variable is UINT (2). The corresponding variable name is not known, but here are its "
                "possible values :'" +
                possibleValues + "'. If you recently added a new parameter in the parameter map, the given type is probably wrong.");
        }
    }
    ParameterInfo(const ParameterType& type, const std::string& possibleValues, std::string* parameterPtr)
        : type(type), possibleValues(possibleValues), parameterPtr((void*)parameterPtr) {
        if (type != STRING) {
            throw std::runtime_error(
                "During the initialization of the library parameter maps, a type mismatch has been found. Apparently, the given "
                "parameterType is: '" +
                std::to_string(type) +
                "' while the type of the parameter variable is STRING (3). The corresponding variable name is not known, but here are its "
                "possible values :'" +
                possibleValues + "'. If you recently added a new parameter in the parameter map, the given type is probably wrong.");
        }
    }
    ParameterInfo(const ParameterType& type, const std::string& possibleValues, float* parameterPtr)
        : type(type), possibleValues(possibleValues), parameterPtr((void*)parameterPtr) {
        if (type != FLOAT) {
            throw std::runtime_error(
                "During the initialization of the library parameter maps, a type mismatch has been found. Apparently, the given "
                "parameterType is: '" +
                std::to_string(type) +
                "' while the type of the parameter variable is FLOAT (4). The corresponding variable name is not known, but here are its "
                "possible values :'" +
                possibleValues + "'. If you recently added a new parameter in the parameter map, the given type is probably wrong.");
        }
    }
    ParameterInfo(const ParameterType& type, const std::string& possibleValues, double* parameterPtr)
        : type(type), possibleValues(possibleValues), parameterPtr((void*)parameterPtr) {
        if (type != DOUBLE) {
            throw std::runtime_error(
                "During the initialization of the library parameter maps, a type mismatch has been found. Apparently, the given "
                "parameterType is: '" +
                std::to_string(type) +
                "' while the type of the parameter variable is DOUBLE (5). The corresponding variable name is not known, but here are its "
                "possible values :'" +
                possibleValues + "'. If you recently added a new parameter in the parameter map, the given type is probably wrong.");
        }
    }
};

using ParameterMap = std::unordered_map<std::string, ParameterInfo>;

void setParameterValue(ParameterMap& parameterMap, const std::string& parameterName, const std::string& parameterValue,
                       bool fromPreset);

}  // namespace uvgutils
