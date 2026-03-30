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

#include "bitstreamGeneration.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "atlas_context.hpp"
#include "utils/parameters.hpp"
#include "uvgutils/log.hpp"
#include "utils/types.hpp"
#include "uvgvpccenc/uvgvpccenc.hpp"
#include "video_sub_bitstream.hpp"
#include "v3cbitstream.hpp"
#include "vps.hpp"
#include "utils/statsCollector.hpp"

/// \file Entry point for the whole bitstream generation process.

using namespace uvgvpcc_enc;

void BitstreamGeneration::createV3CGOFBitstream(const std::shared_ptr<uvgvpcc_enc::GOF>& gofUVG, uvgvpcc_enc::API::v3c_unit_stream* output) {
    uvgutils::Logger::log<uvgutils::LogLevel::INFO>("BITSTREAM GENERATION",
                                                    "GOF " + std::to_string(gofUVG->gofId) + " : Create V3C GOF bitstream using uvgVPCC.\n");

    // --------------- Generate VPS ---------------
    auto v3c_parameter_set = std::make_unique<vps>(gofUVG);

    // --------------- Generate atlas context -------------------
    auto atlas = std::make_unique<atlas_context>();
    atlas->initialize_atlas_context(gofUVG);

    for (auto& frame : gofUVG->frames) {
        frame.reset();  // Release memory
    }

    // --------------- Fetch video sub-bitstream data ---------------------------------------------
    // Occupancy map
    auto bitstream_ovd = std::make_unique<std::vector<uint8_t>>();
    std::vector<nal_info> ovd_nals;  // For low delay bitstreams
    // if (p_->useEncoderCommand) {
    //     read(gofUVG->baseNameOccupancy + ".hevc", *bitstream_ovd.get());
    // }
    *bitstream_ovd.get() = gofUVG->bitstreamOccupancy;
    byteStreamToSampleStream(*bitstream_ovd.get(), 4, ovd_nals, false);
    std::vector<uint8_t>().swap(gofUVG->bitstreamOccupancy);  // Release memory

    // Geometry map
    auto bitstream_gvd = std::make_unique<std::vector<uint8_t>>();
    std::vector<nal_info> gvd_nals;  // For low delay bitstreams
    // if (p_->useEncoderCommand) {
    //     read(gofUVG->baseNameGeometry + ".hevc", *bitstream_gvd.get());
    // }
    *bitstream_gvd.get() = gofUVG->bitstreamGeometry;
    byteStreamToSampleStream(*bitstream_gvd.get(), 4, gvd_nals, false);
    std::vector<uint8_t>().swap(gofUVG->bitstreamGeometry);  // Release memory

    // Attribute map
    auto bitstream_avd = std::make_unique<std::vector<uint8_t>>();
    std::vector<nal_info> avd_nals;  // For low delay bitstreams
    // if (p_->useEncoderCommand) {
    //     read(gofUVG->baseNameAttribute + ".hevc", *bitstream_avd.get());
    // }
    *bitstream_avd.get() = gofUVG->bitstreamAttribute;
    byteStreamToSampleStream(*bitstream_avd.get(), 4, avd_nals, false);
    std::vector<uint8_t>().swap(gofUVG->bitstreamAttribute);  // Release memory

    uvgv3cbitstream::V3cGof gof;
    gof.gof_id = gofUVG->gofId;
    gof.n_frames = gofUVG->nbFrames;
    gof.vps = std::move(v3c_parameter_set);
    gof.atlas = std::move(atlas);
    gof.ovd = std::move(bitstream_ovd);
    gof.gvd = std::move(bitstream_gvd);
    gof.avd = std::move(bitstream_avd);

    // ---------- remove intermediate files ----------
    // if (!p_->exportIntermediateMaps /*&& p_->useEncoderCommand*/) {
    //     Utils::removeFile(gofUVG->baseNameOccupancy + ".hevc");
    //     Utils::removeFile(gofUVG->baseNameGeometry + ".hevc");
    //     Utils::removeFile(gofUVG->baseNameAttribute + ".hevc");
    // }
    const std::vector<uvgv3cbitstream::SerializedUnit> serialized_units =
        p_->lowDelayBitstream ? uvgv3cbitstream::writeV3cLdUnits(gof, ovd_nals, gvd_nals, avd_nals, p_->doubleLayer)
                                   : uvgv3cbitstream::writeV3cUnits(gof);

    uvgvpcc_enc::API::v3c_unit_batch batch;
    batch.v3c_units.reserve(serialized_units.size());
    for (const auto& unit : serialized_units) {
        auto data = std::make_unique<char[]>(unit.len);
        std::copy_n(unit.data.get(), static_cast<std::ptrdiff_t>(unit.len), data.get());
        batch.v3c_units.emplace_back(static_cast<uvgvpcc_enc::API::VUT>(unit.type), unit.len, std::move(data));
    }
    output->io_mutex.lock();
    output->v3c_unit_batches.push(std::move(batch));
    output->io_mutex.unlock();

    if (p_->displayBitstreamGenerationFps) {
        static double lastStampJobCreateV3CGOFBitstream = 0.0;
        const double currentStampJobCreateV3CGOFBitstream = uvgutils::global_timer.elapsed();
        const double ms = currentStampJobCreateV3CGOFBitstream - lastStampJobCreateV3CGOFBitstream;
        double fps = (static_cast<double>(gofUVG->nbFrames) * 1000.0) / ms;
        std::ostringstream msStream;
        msStream << std::fixed << std::setprecision(1) << ms;
        std::string msStr = msStream.str();
        fps = std::round(fps * 10.0) / 10.0;
        std::ostringstream fpsStream;
        fpsStream << std::fixed << std::setprecision(1) << fps;
        std::string fpsStr = fpsStream.str();
        lastStampJobCreateV3CGOFBitstream = currentStampJobCreateV3CGOFBitstream;
        uvgutils::Logger::log<uvgutils::LogLevel::INFO>(
            "BITSTREAM GENERATION", "GOF " + std::to_string(gofUVG->gofId) + " : Delay since last createV3CGOFBitstream: " + msStr +
                                        "ms, GOF with " + std::to_string(gofUVG->nbFrames) + " frames => " + fpsStr + "fps\n");
    }

    output->available_chunks.release();

    if(uvgvpcc_enc::p_->exportStatistics){
        stats.writeToFile(uvgvpcc_enc::p_->statisticsDir + "Statistics.json", gofUVG->gofId);
    }
}
