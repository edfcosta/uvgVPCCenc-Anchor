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

/// \file Main file of the uvgVPCCenc library. Defines the public API.

#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <semaphore>
#include <string>
#include <vector>

#include "uvgformat/uvgFrame.hpp"

namespace uvgvpcc_enc {

/// @brief API of the uvgVPCCenc library
namespace API {

/// @brief Bitstream writing miscellaneous
struct v3c_chunk {
    size_t len = 0;                // Length of data in buffer
    std::unique_ptr<char[]> data;  // Actual data (char type can be used to describe a byte. No need for uint8_t or unsigned char types.)
    std::vector<size_t> v3c_unit_sizes = {};

    v3c_chunk() = default;
    v3c_chunk(size_t len, std::unique_ptr<char[]> data) : len(len), data(std::move(data)) {}
};

// ht: A V3C unit stream is composed of only V3C units without parsing information in the bitstream itself. The parsing information is here
// given separately.
struct v3c_unit_stream {
    size_t v3c_unit_size_precision_bytes = 0;
    std::queue<v3c_chunk> v3c_chunks = {};
    std::counting_semaphore<> available_chunks{0};
    std::mutex io_mutex;  // Locks production and consumption in the v3c_chunks queue
};

void initializeEncoder();
void setParameter(const std::string& parameterName, const std::string& parameterValue);
void encodeFrame(std::shared_ptr<uvgformat::uvgFrame> frame, v3c_unit_stream* output);
void emptyFrameQueue();
void stopEncoder();

}  // namespace API

};  // namespace uvgvpcc_enc
