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

/// \file Internal encoder frame context. Wraps a user-supplied uvgFrame with
/// all encoder-internal state required to process a single point cloud frame.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <semaphore>
#include <vector>

#include "uvgformat/attributeData.hpp"
#include "uvgformat/uvgFrame.hpp"
#include "uvgvpcc/uvgvpcc.hpp"
#include "utils/parameters.hpp"

namespace uvgvpcc_enc {

/// @brief Encoder-internal per-frame state.
///
/// Holds a shared_ptr to the user's uvgFrame (zero-copy ownership) plus all
/// encoder bookkeeping. The reference members pointsGeometry and pointsAttribute
/// are bound at construction to the geometry/attribute vectors inside the
/// uvgFrame payload, providing direct access with the same syntax as the old
/// Frame struct. Both are non-const because the encoder clears their contents
/// after use to release memory (via swap-with-empty).
struct FrameContext {
    // ---- User payload (shared ownership, zero-copy) ----
    // Declared first so references below are bound to valid memory.
    std::shared_ptr<uvgformat::uvgFrame> uvgframe;

    // ---- Convenience references into the payload (non-owning, bound at construction) ----
    std::vector<uvgutils::VectorN<typeGeometryInput, 3>>& pointsGeometry;
    std::vector<uvgutils::VectorN<uint8_t, 3>>& pointsAttribute;

    // ---- Encoder-assigned identity ----
    size_t frameId;
    size_t gofId;
    size_t frameNumber;  // copied from uvgframe->frameNumber

    // ---- Encoder lifecycle ----
    std::weak_ptr<GOF> gof;
    std::shared_ptr<std::counting_semaphore<UINT16_MAX>> conccurentFrameSem;

    // ---- Map sizing ----
    size_t mapHeight;
    size_t mapHeightDS;

    // ---- Centralized memory pointers (set by GOF::setFrameMemoryPtrs) ----
    std::vector<Patch>* patchList;
    std::vector<uint8_t>* occupancyMap;
    std::vector<uint8_t>* occupancyMapDS;
    std::vector<uint8_t>* geometryMapL1;
    std::vector<uint8_t>* geometryMapL2;
    std::vector<uint8_t>* attributeMapL1;
    std::vector<uint8_t>* attributeMapL2;

    FrameContext(const size_t frameId, std::shared_ptr<uvgformat::uvgFrame> frame)
        : uvgframe(std::move(frame))
        , pointsGeometry(uvgformat::getGeometryRgb(uvgframe->attributes).geometry)
        , pointsAttribute(uvgformat::getGeometryRgb(uvgframe->attributes).attribute)
        , frameId(frameId)
        , gofId(0)
        , frameNumber(uvgframe->frameNumber)
        , mapHeight(p_->minimumMapHeight)
        , mapHeightDS(p_->minimumMapHeight / p_->occupancyMapDSResolution)
        , patchList(nullptr)
        , occupancyMap(nullptr)
        , occupancyMapDS(nullptr)
        , geometryMapL1(nullptr)
        , geometryMapL2(nullptr)
        , attributeMapL1(nullptr)
        , attributeMapL2(nullptr) {}

    ~FrameContext() {
        if (conccurentFrameSem) {
            conccurentFrameSem->release();
        }
    }

    void printInfo() const;
};

}  // namespace uvgvpcc_enc
