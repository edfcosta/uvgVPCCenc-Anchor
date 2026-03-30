/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 ****************************************************************************/

#include "vps.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>

using namespace uvgv3cbitstream;

vps::vps(const std::shared_ptr<uvgvpcc_enc::GOF>& gofUVG) {
    if (uvgvpcc_enc::p_->occupancyEncoderName == "Kvazaar" && uvgvpcc_enc::p_->geometryEncoderName == "Kvazaar" &&
        uvgvpcc_enc::p_->attributeEncoderName == "Kvazaar") {
        codec_group_ = 1;
#if LINK_FFMPEG
    } else if (uvgvpcc_enc::p_->occupancyEncoderName == "FFmpeg" || uvgvpcc_enc::p_->geometryEncoderName == "FFmpeg" ||
               uvgvpcc_enc::p_->attributeEncoderName == "FFmpeg") {
        codec_group_ = 1;
#endif
    } else {
        throw std::runtime_error("Error : unknown ptl_profile_codec_group_idc.");
    }

    size_t vps_length_bits = 0;
    ptl_ = fill_ptl(vps_length_bits);
    gofId = gofUVG->gofId;
    vps_v3c_parameter_set_id_ = gofUVG->gofId % 16;
    vps_atlas_count_minus1_ = 0;
    vps_length_bits += 18;

    for (uint8_t k = 0; k < (vps_atlas_count_minus1_ + 1); k++) {
        vps_atlas_id_.push_back(0);
        vps_frame_width_.push_back(uvgvpcc_enc::p_->mapWidth);
        vps_frame_height_.push_back(gofUVG->mapHeightGOF);
        vps_length_bits += 6 + uvg_calculate_ue_len(vps_frame_width_.back()) + uvg_calculate_ue_len(vps_frame_height_.back());

        bool vps_map_count_minus1 = uvgvpcc_enc::p_->doubleLayer;
        vps_map_count_minus1_.push_back(static_cast<uint8_t>(vps_map_count_minus1));
        if (vps_map_count_minus1_.back() > 0) {
            vps_multiple_map_streams_present_flag_.push_back(false);
        }
        vps_map_absolute_coding_enabled_flag_.push_back({true});
        vps_map_predictor_index_diff_.push_back({static_cast<uint16_t>(false)});
        vps_length_bits += 4 + (vps_map_count_minus1_.back() > 0 ? 1 : 0);

        for (uint8_t i = 1; i <= vps_map_count_minus1_.at(k); ++i) {
            vps_map_absolute_coding_enabled_flag_.at(k).push_back(true);
            if (!vps_map_absolute_coding_enabled_flag_.at(k).at(i)) {
                vps_map_predictor_index_diff_.at(k).push_back(static_cast<uint16_t>(false));
                vps_length_bits += uvg_calculate_ue_len(0);
            }
        }

        vps_auxiliary_video_present_flag_.push_back(false);
        vps_occupancy_video_present_flag_.push_back(true);
        vps_geometry_video_present_flag_.push_back(true);
        vps_attribute_video_present_flag_.push_back(true);
        vps_length_bits += 4;

        occupancy_information oi;
        oi.oi_occupancy_codec_id = codec_group_;
        oi.oi_lossy_occupancy_compression_threshold = 0;
        oi.oi_occupancy_2d_bit_depth_minus1 = 7;
        oi.oi_occupancy_MSB_align_flag = false;
        occupancy_info_.push_back(oi);
        vps_length_bits += 22;

        geometry_information gi;
        gi.gi_geometry_codec_id = codec_group_;
        const size_t geometry_nominal_2d_bitdepth = 8;
        gi.gi_geometry_2d_bit_depth_minus1 = static_cast<uint8_t>(geometry_nominal_2d_bitdepth - 1);
        gi.gi_geometry_MSB_align_flag = false;
        gi.gi_geometry_3d_coordinates_bit_depth_minus1 = static_cast<uint8_t>(uvgvpcc_enc::p_->geoBitDepthInput);
        gi.gi_auxiliary_geometry_codec_id = codec_group_;
        geometry_info_.push_back(gi);
        vps_length_bits += 19 + (vps_auxiliary_video_present_flag_.at(k) ? 8 : 0);

        attribute_information ai;
        ai.ai_attribute_count = 1;
        vps_length_bits += 7;
        for (uint8_t i = 0; i < ai.ai_attribute_count; i++) {
            ai.ai_attribute_type_id.push_back(0);
            ai.ai_attribute_codec_id.push_back(codec_group_);
            ai.ai_auxiliary_attribute_codec_id.push_back(codec_group_);
            vps_length_bits += 12 + (vps_auxiliary_video_present_flag_.at(k) ? 8 : 0);
            ai.ai_attribute_map_absolute_coding_persistence_flag.push_back(false);
            ai.ai_attribute_dimension_minus1.push_back(2);
            const uint8_t d = ai.ai_attribute_dimension_minus1.at(i);
            vps_length_bits += (vps_map_count_minus1_.at(k) > 0 ? 1 : 0) + 6;
            if (d != 0) {
                vps_length_bits += 6;
            }
            ai.ai_attribute_dimension_partitions_minus1.push_back(0);
            ai.ai_attribute_partition_channels_minus1.push_back({0});
            ai.ai_attribute_2d_bit_depth_minus1.push_back(7);
            ai.ai_attribute_MSB_align_flag.push_back(false);
            vps_length_bits += 6;
        }
        attribute_info_.push_back(ai);
    }

    vps_length_bits += 1;
    vps_extension_present_flag_ = false;
    vps_packing_information_present_flag_ = false;
    vps_miv_extension_present_flag_ = false;
    vps_extension_6bits_ = 0;
    vps_length_bytes_ = std::ceil(static_cast<float>(vps_length_bits) / 8.F);
    vps_extension_length_minus1_ = 0;
    vps_extension_data_byte_ = 0;
}

profile_tier_level vps::fill_ptl(size_t& len) const {
    profile_tier_level ptl;
    ptl.ptl_tier_flag = false;
    ptl.ptl_profile_codec_group_idc = codec_group_;
    ptl.ptl_profile_toolset_idc = 1;
    ptl.ptl_profile_reconstruction_idc = 1;
    ptl.ptl_max_decodes_idc = 15;
    ptl.ptl_level_idc = 30;
    ptl.ptl_num_sub_profiles = 0;
    ptl.ptl_extended_sub_profile_flag = false;
    ptl.ptl_toolset_constraints_present_flag = false;
    ptl.ptc = profile_toolset_constraints_information();
    ptl.ptc.ptc_one_v3c_frame_only_flag = false;
    ptl.ptc.ptc_eom_constraint_flag = false;
    ptl.ptc.ptc_plr_constraint_flag = false;
    ptl.ptc.ptc_no_eight_orientations_constraint_flag = false;
    ptl.ptc.ptc_no_45degree_projection_patch_constraint_flag = false;

    len += 72 + ptl.ptl_num_sub_profiles * (ptl.ptl_extended_sub_profile_flag ? 64 : 32);
    const size_t ptc_len = 40 + (ptl.ptc.ptc_num_reserved_constraint_bytes * 8);
    len += (ptl.ptl_toolset_constraints_present_flag ? ptc_len : 0);
    return ptl;
}
