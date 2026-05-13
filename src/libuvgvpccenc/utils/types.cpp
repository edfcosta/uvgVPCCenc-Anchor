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

/// \file Implementation of Patch/FrameContext/GOF methods.

#include "types.hpp"
#include "commonMemory.hpp"
#include "uvgutils/log.hpp"

namespace uvgvpcc_enc {

void FrameContext::printInfo() const {
    uvgutils::Logger::log<uvgutils::LogLevel::DEBUG>(
        "FRAME-INFO", "Frame " + std::to_string(frameId) + " :\n" + "\tPath: " + uvgframe->sourcePath + "\n" + "\tFrame Number: " +
                            std::to_string(frameNumber) + "\n" + "\tpointsGeometry size: " + std::to_string(pointsGeometry.size()) + "\n" +
                            "\tpointsAttribute size: " + std::to_string(pointsAttribute.size()) + "\n");
}

GOF::GOF(const size_t& id) : gofId(id) {
    auto& cm = CommonMemory::get();
    framePatches            = cm.getOrCreateFramePatches           (gofId);
    frameOccupancyMaps      = cm.getOrCreateFrameOccupancyMaps     (gofId);
    frameOccupancyMapsColor = cm.getOrCreateFrameOccupancyMapsColor(gofId);
    frameOccupancyMapsDS    = cm.getOrCreateFrameOccupancyMapsDS   (gofId);
    frameGeometryMapsL1     = cm.getOrCreateFrameGeometryMapsL1    (gofId);
    frameGeometryMapsL2     = cm.getOrCreateFrameGeometryMapsL2    (gofId);
    frameAttributeMapsL1    = cm.getOrCreateFrameAttributeMapsL1   (gofId);
    frameAttributeMapsL2    = cm.getOrCreateFrameAttributeMapsL2   (gofId);
}

void GOF::setFrameMemoryPtrs(std::shared_ptr<FrameContext>& frame) {
    const size_t framePos = frame->frameId % p_->sizeGOF;
    frame->patchList = &(*framePatches)[framePos];

    frame->occupancyMap = &(*frameOccupancyMaps)[framePos];
    if(p_->exportIntermediateFiles){
        frame->occupancyMapColored = &(*frameOccupancyMapsColor)[framePos];
    }
    frame->occupancyMapDS = &(*frameOccupancyMapsDS)[framePos];
    frame->geometryMapL1 = &(*frameGeometryMapsL1)[framePos];
    frame->geometryMapL2 = &(*frameGeometryMapsL2)[framePos];
    frame->attributeMapL1 = &(*frameAttributeMapsL1)[framePos];
    frame->attributeMapL2 = &(*frameAttributeMapsL2)[framePos];
}

GOF::~GOF() {
    CommonMemory::get().clearGofMaps(gofId);
}

}  // namespace uvgvpcc_enc
