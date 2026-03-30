/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 *
 * Copyright (c) 2024-present, Tampere University, ITU/ISO/IEC, project contributors
 * All rights reserved.
 ****************************************************************************/

#include "v3cbitstream.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <new>
#include <stdexcept>

namespace uvgv3cbitstream {

namespace {

inline void put_u(bitstream_t* stream, uint32_t data, uint8_t bits) { uvg_bitstream_put(stream, data, bits); }
inline void put_ue(bitstream_t* stream, uint32_t data) { uvg_bitstream_put_ue(stream, data); }

void write_nal_hdr(bitstream_t* stream, uint8_t nal_type, uint8_t nal_layer_id, uint8_t nal_temporal_id_plus1) {
    uvg_bitstream_put(stream, 0, 1);
    uvg_bitstream_put(stream, nal_type, 6);
    uvg_bitstream_put(stream, nal_layer_id, 6);
    uvg_bitstream_put(stream, nal_temporal_id_plus1, 3);
}

void write_atlas_seq_parameter_set(bitstream_t* stream, const AtlasContext& atlas) {
    const auto& asps = atlas.asps_;
    put_ue(stream, asps.asps_atlas_sequence_parameter_set_id);
    put_ue(stream, asps.asps_frame_width);
    put_ue(stream, asps.asps_frame_height);
    put_u(stream, static_cast<uint8_t>(asps.asps_geometry_3d_bit_depth_minus1), 5);
    put_u(stream, static_cast<uint8_t>(asps.asps_geometry_2d_bit_depth_minus1), 5);
    put_ue(stream, static_cast<uint8_t>(asps.asps_log2_max_atlas_frame_order_cnt_lsb_minus4));
    put_ue(stream, asps.asps_max_dec_atlas_frame_buffering_minus1);
    put_u(stream, asps.asps_long_term_ref_atlas_frames_flag, 1);
    put_ue(stream, asps.asps_num_ref_atlas_frame_lists_in_asps);
    for (size_t i = 0; i < asps.asps_num_ref_atlas_frame_lists_in_asps; i++) {
        const ref_list_struct& ref = asps.ref_lists.at(i);
        put_ue(stream, ref.num_ref_entries);
        for (size_t j = 0; j < ref.num_ref_entries; ++j) {
            if (asps.asps_long_term_ref_atlas_frames_flag) {
                put_u(stream, ref.st_ref_atlas_frame_flag.at(j), 1);
            }
            if (ref.st_ref_atlas_frame_flag.at(j)) {
                put_ue(stream, ref.abs_delta_afoc_st.at(j));
                if (ref.abs_delta_afoc_st.at(j) > 0) {
                    put_u(stream, static_cast<uint32_t>(ref.straf_entry_sign_flag.at(j)), 1);
                }
            }
        }
    }
    put_u(stream, asps.asps_use_eight_orientations_flag, 1);
    put_u(stream, asps.asps_extended_projection_enabled_flag, 1);
    if (asps.asps_extended_projection_enabled_flag) {
        put_ue(stream, static_cast<uint32_t>(asps.asps_max_number_projections_minus1));
    }
    put_u(stream, asps.asps_normal_axis_limits_quantization_enabled_flag, 1);
    put_u(stream, asps.asps_normal_axis_max_delta_value_enabled_flag, 1);
    put_u(stream, asps.asps_patch_precedence_order_flag, 1);
    put_u(stream, asps.asps_log2_patch_packing_block_size, 3);
    put_u(stream, asps.asps_patch_size_quantizer_present_flag, 1);
    put_u(stream, asps.asps_map_count_minus1, 4);
    put_u(stream, asps.asps_pixel_deinterleaving_enabled_flag, 1);
    if (asps.asps_pixel_deinterleaving_enabled_flag) {
        for (size_t j = 0; j < asps.asps_map_count_minus1; ++j) {
            put_u(stream, static_cast<uint32_t>(asps.asps_map_pixel_deinterleaving_flag.at(j)), 1);
        }
    }
    put_u(stream, asps.asps_raw_patch_enabled_flag, 1);
    put_u(stream, asps.asps_eom_patch_enabled_flag, 1);
    if (asps.asps_eom_patch_enabled_flag && asps.asps_map_count_minus1 == 0) {
        put_u(stream, asps.asps_eom_fix_bit_count_minus1, 4);
    }
    if (asps.asps_raw_patch_enabled_flag || asps.asps_eom_patch_enabled_flag) {
        put_u(stream, asps.asps_auxiliary_video_enabled_flag, 4);
    }
    put_u(stream, asps.asps_plr_enabled_flag, 1);
    put_u(stream, asps.asps_vui_parameters_present_flag, 1);
    put_u(stream, asps.asps_extension_present_flag, 1);
    if (asps.asps_extension_present_flag) {
        put_u(stream, asps.asps_vpcc_extension_present_flag, 1);
        put_u(stream, asps.asps_miv_extension_present_flag, 1);
        put_u(stream, asps.asps_extension_6bits, 6);
    }
    if (asps.asps_vpcc_extension_present_flag) {
        put_u(stream, asps.asps_vpcc_remove_duplicate_point_enabled_flag, 1);
        if (asps.asps_pixel_deinterleaving_enabled_flag || asps.asps_plr_enabled_flag) {
            put_ue(stream, asps.asps_vpcc_surface_thickness_minus1);
        }
    }
    uvg_bitstream_align(stream);
}

void write_atlas_frame_parameter_set(bitstream_t* stream, const AtlasContext& atlas) {
    const auto& afps = atlas.afps_;
    put_ue(stream, afps.afps_atlas_frame_parameter_set_id);
    put_ue(stream, afps.afps_atlas_sequence_parameter_set_id);
    put_u(stream, afps.afti.afti_single_tile_in_atlas_frame_flag, 1);
    put_u(stream, afps.afti.afti_signalled_tile_id_flag, 1);
    put_u(stream, afps.afps_output_flag_present_flag, 1);
    put_ue(stream, afps.afps_num_ref_idx_default_active_minus1);
    put_ue(stream, afps.afps_additional_lt_afoc_lsb_len);
    put_u(stream, afps.afps_lod_mode_enabled_flag, 1);
    put_u(stream, afps.afps_raw_3d_offset_bit_count_explicit_mode_flag, 1);
    put_u(stream, afps.afps_extension_present_flag, 1);
    put_u(stream, afps.afps_miv_extension_present_flag, 1);
    put_u(stream, afps.afps_extension_7bits, 7);
    uvg_bitstream_align(stream);
}

void write_atlas_tile_header(bitstream_t* stream, const AtlasContext& atlas, NAL nalu_t, const atlas_tile_header& ath) {
    const auto& asps = atlas.asps_;
    const auto& afps = atlas.afps_;

    if (nalu_t >= NAL_GBLA_W_LP && nalu_t <= NAL_RSV_IRAP_ACL_29) {
        put_u(stream, ath.ath_no_output_of_prior_atlas_frames_flag, 1);
    }
    put_ue(stream, ath.ath_atlas_frame_parameter_set_id);
    put_ue(stream, ath.ath_atlas_adaptation_parameter_set_id);
    put_u(stream, ath.ath_id, 0);
    put_ue(stream, ath.ath_type);
    if (afps.afps_output_flag_present_flag) {
        put_u(stream, ath.ath_atlas_output_flag, 1);
    }
    const size_t log2_max_atlas_frm_order_cnt_lsb = asps.asps_log2_max_atlas_frame_order_cnt_lsb_minus4 + 4;
    put_u(stream, static_cast<uint32_t>(ath.ath_atlas_frm_order_cnt_lsb), static_cast<uint8_t>(log2_max_atlas_frm_order_cnt_lsb));
    if (asps.asps_num_ref_atlas_frame_lists_in_asps > 0) {
        put_u(stream, ath.ath_ref_atlas_frame_list_asps_flag, 1);
    }
    if (!ath.ath_ref_atlas_frame_list_asps_flag) {
        throw std::runtime_error("Atlas serialization requires ASPS reference lists");
    }
    if (asps.asps_num_ref_atlas_frame_lists_in_asps > 1) {
        const size_t bit_len = std::ceil(std::log2(asps.asps_num_ref_atlas_frame_lists_in_asps));
        put_u(stream, ath.ath_ref_atlas_frame_list_idx, static_cast<uint8_t>(bit_len));
    }
    const size_t num_ltr_atlas_frm_entries = 0;
    for (size_t j = 0; j < num_ltr_atlas_frm_entries; j++) {
        put_u(stream, ath.ath_additional_afoc_lsb_present_flag.at(j), 1);
        if (ath.ath_additional_afoc_lsb_present_flag.at(j)) {
            put_u(stream, ath.ath_additional_afoc_lsb_val.at(j), afps.afps_additional_lt_afoc_lsb_len);
        }
    }
    if (ath.ath_type != SKIP_TILE) {
        if (asps.asps_normal_axis_limits_quantization_enabled_flag) {
            put_u(stream, ath.ath_pos_min_d_quantizer, 5);
            if (asps.asps_normal_axis_max_delta_value_enabled_flag) {
                put_u(stream, ath.ath_pos_delta_max_d_quantizer, 5);
            }
        }
        if (asps.asps_patch_size_quantizer_present_flag) {
            put_u(stream, ath.ath_patch_size_x_info_quantizer, 3);
            put_u(stream, ath.ath_patch_size_y_info_quantizer, 3);
        }
        if (afps.afps_raw_3d_offset_bit_count_explicit_mode_flag) {
            const size_t bit_len = std::floor(std::log2(asps.asps_geometry_3d_bit_depth_minus1 + 1));
            put_u(stream, ath.ath_raw_3d_offset_axis_bit_count_minus1, static_cast<uint8_t>(bit_len));
        }
        if (ath.ath_type == ATH::P_TILE && num_ltr_atlas_frm_entries > 1) {
            put_u(stream, ath.ath_num_ref_idx_active_override_flag, 1);
            if (ath.ath_num_ref_idx_active_override_flag) {
                put_ue(stream, ath.ath_num_ref_idx_active_minus1);
            }
        }
    }
    uvg_bitstream_align(stream);
}

void write_patch_data_unit(bitstream_t* stream, const AtlasContext& atlas, const patch_data_unit& pdu, const atlas_tile_header& ath) {
    const auto& asps = atlas.asps_;
    const auto& afps = atlas.afps_;
    uvg_bitstream_put_ue(stream, static_cast<uint32_t>(pdu.pdu_2d_pos_x));
    uvg_bitstream_put_ue(stream, static_cast<uint32_t>(pdu.pdu_2d_pos_y));
    uvg_bitstream_put_ue(stream, static_cast<uint32_t>(pdu.pdu_2d_size_x_minus1));
    uvg_bitstream_put_ue(stream, static_cast<uint32_t>(pdu.pdu_2d_size_y_minus1));
    uvg_bitstream_put(stream, static_cast<uint32_t>(pdu.pdu_3d_offset_u), asps.asps_geometry_3d_bit_depth_minus1 + 1);
    uvg_bitstream_put(stream, static_cast<uint32_t>(pdu.pdu_3d_offset_v), asps.asps_geometry_3d_bit_depth_minus1 + 1);
    uvg_bitstream_put(stream, static_cast<uint32_t>(pdu.pdu_3d_offset_d), asps.asps_geometry_3d_bit_depth_minus1 - ath.ath_pos_min_d_quantizer + 1);
    if (asps.asps_normal_axis_max_delta_value_enabled_flag) {
        const uint32_t range_d_bit_depth = std::min(asps.asps_geometry_2d_bit_depth_minus1, asps.asps_geometry_3d_bit_depth_minus1) + 1;
        uvg_bitstream_put(stream, static_cast<uint32_t>(pdu.pdu_3d_range_d), range_d_bit_depth - ath.ath_pos_delta_max_d_quantizer);
    }
    uvg_bitstream_put(stream, static_cast<uint32_t>(pdu.pdu_projection_id), static_cast<uint8_t>(std::ceil(std::log2(6))));
    uvg_bitstream_put(stream, static_cast<uint32_t>(pdu.pdu_orientation_index), 1);
    if (afps.afps_lod_mode_enabled_flag) {
        uvg_bitstream_put(stream, static_cast<uint32_t>(pdu.pdu_lod_enabled_flag), 1);
        if (pdu.pdu_lod_enabled_flag) {
            uvg_bitstream_put_ue(stream, pdu.pdu_lod_scale_x_minus1);
            uvg_bitstream_put_ue(stream, pdu.pdu_lod_scale_y_idc);
        }
    }
}

void write_atlas_tile_data_unit(bitstream_t* stream, const AtlasContext& atlas, const atlas_tile_data_unit& atdu, const atlas_tile_header& ath) {
    if (ath.ath_type == SKIP_TILE) {
        return;
    }
    for (size_t pu_count = 0; pu_count < atdu.patch_information_data_.size(); pu_count++) {
        uvg_bitstream_put_ue(stream, atdu.patch_information_data_.at(pu_count).patchMode);
        if (atdu.patch_information_data_.at(pu_count).patchMode == APM::I_END) {
            break;
        }
        const patch_information_data& pid = atdu.patch_information_data_.at(pu_count);
        assert(ath.ath_type == I_TILE);
        assert(pid.patchMode == I_INTRA);
        write_patch_data_unit(stream, atlas, pid.patch_data_unit_, ath);
    }
}

void write_atlas_tile_layer_rbsp(bitstream_t* stream, const AtlasContext& atlas, NAL nalu_t, const atlas_tile_layer_rbsp& rbsp) {
    write_atlas_tile_header(stream, atlas, nalu_t, rbsp.ath_);
    write_atlas_tile_data_unit(stream, atlas, rbsp.atdu_, rbsp.ath_);
    uvg_bitstream_add_rbsp_trailing_bits(stream);
}

template <typename Writer>
SerializedUnit create_unit(VUT type, Writer&& writer) {
    auto* stream = new bitstream_t;
    uvg_bitstream_init(stream);
    writer(stream);

    SerializedUnit unit;
    unit.type = type;
    unit.len = uvg_bitstream_tell(stream) / 8;
    unit.data = std::make_unique<char[]>(unit.len);

    uvg_data_chunk* data_out = stream->first;
    uint64_t written = 0;
    if (data_out != nullptr) {
        for (uvg_data_chunk* chunk = data_out; chunk != nullptr; chunk = chunk->next) {
            memcpy(unit.data.get() + written, &chunk->data, chunk->len);
            written += chunk->len;
        }
    }
    uvg_bitstream_finalize(stream);
    if (unit.len != written) {
        throw std::runtime_error("Bitstream writing : Error: unit.len != written");
    }
    return unit;
}

void write_common_v3c_header(bitstream_t* stream, VUT unit_type, size_t vps_id) {
    uvg_bitstream_put(stream, unit_type, 5);
    uvg_bitstream_put(stream, static_cast<uint32_t>(vps_id), 4);
    uvg_bitstream_put(stream, 0, 6);
}

}  // namespace

bool writeVps(bitstream_t* stream, const Vps& vps) {
    put_u(stream, vps.ptl_.ptl_tier_flag, 1);
    put_u(stream, vps.ptl_.ptl_profile_codec_group_idc, 7);
    put_u(stream, vps.ptl_.ptl_profile_toolset_idc, 8);
    put_u(stream, vps.ptl_.ptl_profile_reconstruction_idc, 8);
    put_u(stream, 0, 16);
    put_u(stream, vps.ptl_.ptl_max_decodes_idc, 4);
    put_u(stream, 0xfff, 12);
    put_u(stream, vps.ptl_.ptl_level_idc, 8);
    put_u(stream, vps.ptl_.ptl_num_sub_profiles, 6);
    put_u(stream, vps.ptl_.ptl_extended_sub_profile_flag, 1);
    put_u(stream, vps.ptl_.ptl_toolset_constraints_present_flag, 1);
    put_u(stream, vps.vps_v3c_parameter_set_id_, 4);
    put_u(stream, 0, 8);
    put_u(stream, vps.vps_atlas_count_minus1_, 6);

    for (uint8_t j = 0; j < (vps.vps_atlas_count_minus1_ + 1); j++) {
        put_u(stream, j, 6);
        put_ue(stream, static_cast<uint32_t>(vps.vps_frame_width_.at(j)));
        put_ue(stream, static_cast<uint32_t>(vps.vps_frame_height_.at(j)));
        put_u(stream, vps.vps_map_count_minus1_.at(j), 4);
        if (vps.vps_map_count_minus1_.at(j) > 0) {
            put_u(stream, static_cast<uint32_t>(vps.vps_multiple_map_streams_present_flag_.at(j)), 1);
        }
        put_u(stream, static_cast<uint32_t>(vps.vps_auxiliary_video_present_flag_.at(j)), 1);
        put_u(stream, static_cast<uint32_t>(vps.vps_occupancy_video_present_flag_.at(j)), 1);
        put_u(stream, static_cast<uint32_t>(vps.vps_geometry_video_present_flag_.at(j)), 1);
        put_u(stream, static_cast<uint32_t>(vps.vps_attribute_video_present_flag_.at(j)), 1);
        if (vps.vps_occupancy_video_present_flag_.at(j)) {
            put_u(stream, vps.occupancy_info_.at(j).oi_occupancy_codec_id, 8);
            put_u(stream, vps.occupancy_info_.at(j).oi_lossy_occupancy_compression_threshold, 8);
            put_u(stream, vps.occupancy_info_.at(j).oi_occupancy_2d_bit_depth_minus1, 5);
            put_u(stream, vps.occupancy_info_.at(j).oi_occupancy_MSB_align_flag, 1);
        }
        if (vps.vps_geometry_video_present_flag_.at(j)) {
            put_u(stream, vps.geometry_info_.at(j).gi_geometry_codec_id, 8);
            put_u(stream, vps.geometry_info_.at(j).gi_geometry_2d_bit_depth_minus1, 5);
            put_u(stream, vps.geometry_info_.at(j).gi_geometry_MSB_align_flag, 1);
            put_u(stream, vps.geometry_info_.at(j).gi_geometry_3d_coordinates_bit_depth_minus1, 5);
            if (vps.vps_auxiliary_video_present_flag_.at(j)) {
                put_u(stream, vps.geometry_info_.at(j).gi_auxiliary_geometry_codec_id, 8);
            }
        }
        if (vps.vps_attribute_video_present_flag_.at(j)) {
            put_u(stream, vps.attribute_info_.at(j).ai_attribute_count, 7);
            for (uint8_t i = 0; i < vps.attribute_info_.at(j).ai_attribute_count; ++i) {
                put_u(stream, vps.attribute_info_.at(j).ai_attribute_type_id.at(i), 4);
                put_u(stream, vps.attribute_info_.at(j).ai_attribute_codec_id.at(i), 8);
                if (vps.vps_auxiliary_video_present_flag_.at(j)) {
                    put_u(stream, vps.attribute_info_.at(j).ai_auxiliary_attribute_codec_id.at(i), 8);
                }
                if (vps.vps_map_count_minus1_.at(j) > 0) {
                    put_u(stream, static_cast<uint32_t>(vps.attribute_info_.at(j).ai_attribute_map_absolute_coding_persistence_flag.at(i)), 1);
                }
                uint8_t d = vps.attribute_info_.at(j).ai_attribute_dimension_minus1.at(i);
                put_u(stream, d, 6);
                uint8_t m = 0;
                if (d != 0) {
                    m = vps.attribute_info_.at(j).ai_attribute_dimension_partitions_minus1.at(i);
                    put_u(stream, m, 6);
                }
                for (uint8_t k = 0; k < m; k++) {
                    const uint16_t n = vps.attribute_info_.at(j).ai_attribute_partition_channels_minus1.at(i).at(k);
                    put_ue(stream, n);
                    d -= n + 1;
                }
                put_u(stream, vps.attribute_info_.at(j).ai_attribute_2d_bit_depth_minus1.at(i), 5);
                put_u(stream, static_cast<uint32_t>(vps.attribute_info_.at(j).ai_attribute_MSB_align_flag.at(i)), 1);
            }
        }
        put_u(stream, vps.vps_extension_present_flag_, 1);
        if (vps.vps_extension_present_flag_) {
            put_u(stream, vps.vps_packing_information_present_flag_, 1);
            put_u(stream, vps.vps_miv_extension_present_flag_, 1);
            put_u(stream, vps.vps_extension_6bits_, 6);
        }
        uvg_bitstream_align(stream);
    }
    return true;
}

void prepareAtlasContext(AtlasContext& atlas) {
    bitstream_t temp_bitstream;
    uvg_bitstream_init(&temp_bitstream);
    uint32_t nal_max_size = 0;
    atlas.atlas_sub_size_ = 1;
    uint32_t previous_bitstream_size = 0;
    uint32_t current_bitstream_size = 0;
    uint32_t current_nal_size = 0;
    atlas.ad_nal_sizes_.clear();
    atlas.ad_nal_precision_ = 0;

    write_nal_hdr(&temp_bitstream, NAL_ASPS, 0, 1);
    write_atlas_seq_parameter_set(&temp_bitstream, atlas);
    current_bitstream_size = uvg_bitstream_tell(&temp_bitstream) / 8;
    current_nal_size = current_bitstream_size - previous_bitstream_size;
    atlas.ad_nal_sizes_.push_back(current_nal_size);
    atlas.atlas_sub_size_ += current_nal_size;
    previous_bitstream_size = current_bitstream_size;

    write_nal_hdr(&temp_bitstream, NAL_AFPS, 0, 1);
    write_atlas_frame_parameter_set(&temp_bitstream, atlas);
    current_bitstream_size = uvg_bitstream_tell(&temp_bitstream) / 8;
    current_nal_size = current_bitstream_size - previous_bitstream_size;
    atlas.ad_nal_sizes_.push_back(current_nal_size);
    atlas.atlas_sub_size_ += current_nal_size;
    previous_bitstream_size = current_bitstream_size;

    for (size_t i = 0; i < atlas.atlas_data_.size(); ++i) {
        write_nal_hdr(&temp_bitstream, NAL_IDR_N_LP, 0, 1);
        write_atlas_tile_layer_rbsp(&temp_bitstream, atlas, NAL_IDR_N_LP, atlas.atlas_data_.at(i));
        current_bitstream_size = uvg_bitstream_tell(&temp_bitstream) / 8;
        current_nal_size = current_bitstream_size - previous_bitstream_size;
        atlas.ad_nal_sizes_.push_back(current_nal_size);
        atlas.atlas_sub_size_ += current_nal_size;
        nal_max_size = std::max(nal_max_size, current_nal_size);
        previous_bitstream_size = current_bitstream_size;
    }

    const uint32_t nal_precision_minus1 =
        static_cast<uint32_t>(std::min(std::max(static_cast<int>(std::ceil(static_cast<double>(ceilLog2(nal_max_size + 1)) / 8.0)), 1), 8) - 1);
    atlas.ad_nal_precision_ = nal_precision_minus1 + 1;
    atlas.atlas_sub_size_ += 2 * atlas.ad_nal_precision_ + atlas.atlas_data_.size() * atlas.ad_nal_precision_;
    atlas.atlas_sub_size_ += atlas.ad_nal_precision_ + 2;
    uvg_bitstream_clear(&temp_bitstream);
}

void writeAtlasSubBitstream(bitstream_t* stream, const AtlasContext& atlas) {
    const uint32_t nal_precision_in_bits = atlas.ad_nal_precision_ * 8;
    uvg_bitstream_put(stream, atlas.ad_nal_precision_ - 1, 3);
    uvg_bitstream_put(stream, 0, 5);
    uvg_bitstream_put(stream, static_cast<uint32_t>(atlas.ad_nal_sizes_.at(0)), nal_precision_in_bits);
    write_nal_hdr(stream, NAL_ASPS, 0, 1);
    write_atlas_seq_parameter_set(stream, atlas);
    uvg_bitstream_put(stream, static_cast<uint32_t>(atlas.ad_nal_sizes_.at(1)), nal_precision_in_bits);
    write_nal_hdr(stream, NAL_AFPS, 0, 1);
    write_atlas_frame_parameter_set(stream, atlas);
    for (size_t i = 0; i < atlas.atlas_data_.size(); ++i) {
        uvg_bitstream_put(stream, static_cast<uint32_t>(atlas.ad_nal_sizes_.at(i + 2)), nal_precision_in_bits);
        write_nal_hdr(stream, NAL_IDR_N_LP, 0, 1);
        write_atlas_tile_layer_rbsp(stream, atlas, NAL_IDR_N_LP, atlas.atlas_data_.at(i));
    }
    uvg_bitstream_put(stream, 2, nal_precision_in_bits);
    write_nal_hdr(stream, NAL_EOB, 0, 1);
}

void writeAtlasParameterSetNals(bitstream_t* stream, const AtlasContext& atlas) {
    const uint32_t nal_precision_in_bits = atlas.ad_nal_precision_ * 8;
    uvg_bitstream_put(stream, static_cast<uint32_t>(atlas.ad_nal_sizes_.at(0)), nal_precision_in_bits);
    write_nal_hdr(stream, NAL_ASPS, 0, 1);
    write_atlas_seq_parameter_set(stream, atlas);
    uvg_bitstream_put(stream, static_cast<uint32_t>(atlas.ad_nal_sizes_.at(1)), nal_precision_in_bits);
    write_nal_hdr(stream, NAL_AFPS, 0, 1);
    write_atlas_frame_parameter_set(stream, atlas);
}

void writeAtlasNal(bitstream_t* stream, const AtlasContext& atlas, size_t index) {
    const uint32_t nal_precision_in_bits = atlas.ad_nal_precision_ * 8;
    uvg_bitstream_put(stream, static_cast<uint32_t>(atlas.ad_nal_sizes_.at(index + 2)), nal_precision_in_bits);
    write_nal_hdr(stream, NAL_IDR_N_LP, 0, 1);
    write_atlas_tile_layer_rbsp(stream, atlas, NAL_IDR_N_LP, atlas.atlas_data_.at(index));
}

void writeAtlasEob(bitstream_t* stream, const AtlasContext& atlas) {
    const uint32_t nal_precision_in_bits = atlas.ad_nal_precision_ * 8;
    uvg_bitstream_put(stream, 2, nal_precision_in_bits);
    write_nal_hdr(stream, NAL_EOB, 0, 1);
}

std::vector<SerializedUnit> writeV3cUnits(const V3cGof& gof) {
    std::vector<SerializedUnit> units;
    const size_t vps_id = gof.gof_id % 16;
    units.push_back(create_unit(uvgv3cbitstream::V3C_VPS, [&](bitstream_t* stream) {
        uvg_bitstream_put(stream, 0, 32);
        writeVps(stream, *gof.vps);
    }));
    units.push_back(create_unit(uvgv3cbitstream::V3C_AD, [&](bitstream_t* stream) {
        write_common_v3c_header(stream, VUT::V3C_AD, vps_id);
        uvg_bitstream_put(stream, 0, 17);
        writeAtlasSubBitstream(stream, *gof.atlas);
    }));
    units.push_back(create_unit(uvgv3cbitstream::V3C_OVD, [&](bitstream_t* stream) {
        write_common_v3c_header(stream, VUT::V3C_OVD, vps_id);
        uvg_bitstream_put(stream, 0, 17);
        uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.ovd->data()), gof.ovd->size());
    }));
    units.push_back(create_unit(uvgv3cbitstream::V3C_GVD, [&](bitstream_t* stream) {
        write_common_v3c_header(stream, VUT::V3C_GVD, vps_id);
        uvg_bitstream_put(stream, 0, 4);
        uvg_bitstream_put(stream, 0, 1);
        uvg_bitstream_put(stream, 0, 12);
        uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.gvd->data()), gof.gvd->size());
    }));
    units.push_back(create_unit(uvgv3cbitstream::V3C_AVD, [&](bitstream_t* stream) {
        write_common_v3c_header(stream, VUT::V3C_AVD, vps_id);
        uvg_bitstream_put(stream, 0, 7);
        uvg_bitstream_put(stream, 0, 5);
        uvg_bitstream_put(stream, 0, 4);
        uvg_bitstream_put(stream, 0, 1);
        uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.avd->data()), gof.avd->size());
    }));
    return units;
}

std::vector<SerializedUnit> writeV3cLdUnits(const V3cGof& gof, const std::vector<nal_info>& ovd_nals, const std::vector<nal_info>& gvd_nals,
                                            const std::vector<nal_info>& avd_nals, bool double_layer) {
    std::vector<SerializedUnit> units;
    const size_t vps_id = gof.gof_id % 16;
    units.push_back(create_unit(uvgv3cbitstream::V3C_VPS, [&](bitstream_t* stream) {
        uvg_bitstream_put(stream, 0, 32);
        writeVps(stream, *gof.vps);
    }));

    size_t ovd_idx = 4;
    size_t gvd_idx = 4;
    size_t avd_idx = 4;
    for (size_t k = 0; k < gof.n_frames; ++k) {
        units.push_back(create_unit(uvgv3cbitstream::V3C_AD, [&](bitstream_t* stream) {
            write_common_v3c_header(stream, VUT::V3C_AD, vps_id);
            uvg_bitstream_put(stream, 0, 17);
            uvg_bitstream_put(stream, gof.atlas->ad_nal_precision_ - 1, 3);
            uvg_bitstream_put(stream, 0, 5);
            if (k == 0) {
                writeAtlasParameterSetNals(stream, *gof.atlas);
            }
            writeAtlasNal(stream, *gof.atlas, k);
            writeAtlasEob(stream, *gof.atlas);
        }));
        units.push_back(create_unit(uvgv3cbitstream::V3C_OVD, [&](bitstream_t* stream) {
            write_common_v3c_header(stream, VUT::V3C_OVD, vps_id);
            uvg_bitstream_put(stream, 0, 17);
            if (k == 0) {
                for (size_t n = 0; n < 4; n++) {
                    uvg_bitstream_put(stream, static_cast<uint32_t>(ovd_nals.at(n).size), 32);
                    uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.ovd->data() + ovd_nals.at(n).location), ovd_nals.at(n).size);
                }
            }
            uvg_bitstream_put(stream, static_cast<uint32_t>(ovd_nals.at(ovd_idx).size), 32);
            uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.ovd->data() + ovd_nals.at(ovd_idx).location), ovd_nals.at(ovd_idx).size);
        }));
        ovd_idx++;
        units.push_back(create_unit(uvgv3cbitstream::V3C_GVD, [&](bitstream_t* stream) {
            write_common_v3c_header(stream, VUT::V3C_GVD, vps_id);
            uvg_bitstream_put(stream, 0, 4);
            uvg_bitstream_put(stream, 0, 1);
            uvg_bitstream_put(stream, 0, 12);
            if (k == 0) {
                for (size_t n = 0; n < 4; n++) {
                    uvg_bitstream_put(stream, static_cast<uint32_t>(gvd_nals.at(n).size), 32);
                    uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.gvd->data() + gvd_nals.at(n).location), gvd_nals.at(n).size);
                }
            }
            uvg_bitstream_put(stream, static_cast<uint32_t>(gvd_nals.at(gvd_idx).size), 32);
            uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.gvd->data() + gvd_nals.at(gvd_idx).location), gvd_nals.at(gvd_idx).size);
            if (double_layer) {
                uvg_bitstream_put(stream, static_cast<uint32_t>(gvd_nals.at(gvd_idx + 1).size), 32);
                uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.gvd->data() + gvd_nals.at(gvd_idx + 1).location),
                                         gvd_nals.at(gvd_idx + 1).size);
            }
        }));
        gvd_idx += double_layer ? 2 : 1;
        units.push_back(create_unit(uvgv3cbitstream::V3C_AVD, [&](bitstream_t* stream) {
            write_common_v3c_header(stream, VUT::V3C_AVD, vps_id);
            uvg_bitstream_put(stream, 0, 7);
            uvg_bitstream_put(stream, 0, 5);
            uvg_bitstream_put(stream, 0, 4);
            uvg_bitstream_put(stream, 0, 1);
            if (k == 0) {
                for (size_t n = 0; n < 4; n++) {
                    uvg_bitstream_put(stream, static_cast<uint32_t>(avd_nals.at(n).size), 32);
                    uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.avd->data() + avd_nals.at(n).location), avd_nals.at(n).size);
                }
            }
            uvg_bitstream_put(stream, static_cast<uint32_t>(avd_nals.at(avd_idx).size), 32);
            uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.avd->data() + avd_nals.at(avd_idx).location), avd_nals.at(avd_idx).size);
            if (double_layer) {
                uvg_bitstream_put(stream, static_cast<uint32_t>(avd_nals.at(avd_idx + 1).size), 32);
                uvg_bitstream_copy_bytes(stream, reinterpret_cast<const uint8_t*>(gof.avd->data() + avd_nals.at(avd_idx + 1).location),
                                         avd_nals.at(avd_idx + 1).size);
            }
        }));
        avd_idx += double_layer ? 2 : 1;
    }
    return units;
}

}  // namespace uvgv3cbitstream
