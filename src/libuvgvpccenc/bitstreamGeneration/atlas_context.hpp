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

#pragma once

/// \file Handle atlas information based on TMC2 implementation.

#include "utils/parameters.hpp"
#include "utils/types.hpp"
#include "v3cbitstream.hpp"

/* Atlas context is used to hold the atlas data (inside V3C_AD unit) of a single GOF */
class atlas_context : public uvgv3cbitstream::AtlasContext {
   public:
    /* Fill data structures with encoded data from gofUVG and sequence parameters from p_ */
    void initialize_atlas_context(const std::shared_ptr<uvgvpcc_enc::GOF>& gofUVG);

   private:
    // -------------- Functions to fill data structure values --------------
    static uvgv3cbitstream::atlas_sequence_parameter_set create_atlas_sequence_parameter_set(const std::shared_ptr<uvgvpcc_enc::GOF>& gofUVG);
    uvgv3cbitstream::atlas_frame_parameter_set create_atlas_frame_parameter_set();
    uvgv3cbitstream::atlas_frame_tile_information create_atlas_frame_tile_information() const;
    uvgv3cbitstream::atlas_tile_header create_atlas_tile_header(size_t frameIndex, size_t tileIndex) const;
    uvgv3cbitstream::atlas_tile_data_unit create_atlas_tile_data_unit(const std::shared_ptr<uvgvpcc_enc::FrameContext>& frameUVG,
                                                                      uvgv3cbitstream::atlas_tile_header& ath) const;
    uvgv3cbitstream::atlas_tile_layer_rbsp create_atlas_tile_layer_rbsp(size_t frameIndex, size_t tileIndex,
                                                                        const std::shared_ptr<uvgvpcc_enc::FrameContext>& frameUVG);
};
