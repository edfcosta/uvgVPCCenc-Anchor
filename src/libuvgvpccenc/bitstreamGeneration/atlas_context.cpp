/*****************************************************************************
 * This file is part of uvgVPCCenc V-PCC encoder.
 ****************************************************************************/

#include "atlas_context.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

using namespace uvgv3cbitstream;

atlas_tile_header atlas_context::create_atlas_tile_header(size_t frameIndex, size_t tileIndex) const {
    atlas_tile_header ath;
    ath.ath_no_output_of_prior_atlas_frames_flag = false;
    ath.ath_atlas_frame_parameter_set_id = 0;
    ath.ath_atlas_adaptation_parameter_set_id = 0;
    ath.ath_id = tileIndex;
    ath.ath_type = ATH::I_TILE;
    if (afps_.afps_output_flag_present_flag) {
        ath.ath_atlas_output_flag = false;
    }
    const size_t log2_max_atlas_frm_order_cnt_lsb = asps_.asps_log2_max_atlas_frame_order_cnt_lsb_minus4 + 4;
    ath.ath_atlas_frm_order_cnt_lsb = frameIndex % (static_cast<size_t>(1) << log2_max_atlas_frm_order_cnt_lsb);
    ath.ath_ref_atlas_frame_list_asps_flag = asps_.asps_num_ref_atlas_frame_lists_in_asps > 0;
    if (asps_.asps_num_ref_atlas_frame_lists_in_asps > 1) {
        ath.ath_ref_atlas_frame_list_idx = 0;
    }
    const size_t num_ltr_atlas_frm_entries = 1;
    for (size_t j = 0; j < num_ltr_atlas_frm_entries; j++) {
        ath.ath_additional_afoc_lsb_present_flag.push_back(false);
        if (ath.ath_additional_afoc_lsb_present_flag.back()) {
            ath.ath_additional_afoc_lsb_val.push_back(0);
        }
    }
    if (ath.ath_type != ATH::SKIP_TILE) {
        if (asps_.asps_normal_axis_limits_quantization_enabled_flag) {
            ath.ath_pos_min_d_quantizer = static_cast<uint8_t>(std::log2(uvgvpcc_enc::p_->minLevel));
            if (asps_.asps_normal_axis_max_delta_value_enabled_flag) {
                ath.ath_pos_delta_max_d_quantizer = static_cast<uint8_t>(std::log2(uvgvpcc_enc::p_->minLevel));
            }
        }
        if (asps_.asps_patch_size_quantizer_present_flag) {
            ath.ath_patch_size_x_info_quantizer = uvgvpcc_enc::p_->log2QuantizerSizeX;
            ath.ath_patch_size_y_info_quantizer = uvgvpcc_enc::p_->log2QuantizerSizeY;
        }
        const size_t geometry_nominal_2d_bitdepth = 8;
        if (afps_.afps_raw_3d_offset_bit_count_explicit_mode_flag) {
            ath.ath_raw_3d_offset_axis_bit_count_minus1 = uvgvpcc_enc::p_->geoBitDepthInput + 1 - geometry_nominal_2d_bitdepth - 1;
        }
        if (ath.ath_type == ATH::P_TILE && num_ltr_atlas_frm_entries > 1) {
            ath.ath_num_ref_idx_active_override_flag = false;
            if (ath.ath_num_ref_idx_active_override_flag) {
                ath.ath_num_ref_idx_active_minus1 = 0;
            }
        }
    }
    return ath;
}

atlas_tile_data_unit atlas_context::create_atlas_tile_data_unit(const std::shared_ptr<uvgvpcc_enc::FrameContext>& frameUVG,
                                                                atlas_tile_header& ath) const {
    atlas_tile_data_unit atdu;
    const size_t level_of_detail_x = 1;
    const size_t level_of_detail_y = 1;

    for (size_t patch_index = 0; patch_index < (*frameUVG->patchList).size(); ++patch_index) {
        const uvgvpcc_enc::Patch& patchUVG = (*frameUVG->patchList)[patch_index];
        patch_information_data pid;
        pid.patchMode = static_cast<uint8_t>(APM::I_INTRA);
        patch_data_unit& pdu = pid.patch_data_unit_;
        pdu.pdu_2d_pos_x = patchUVG.omPPPosX_;
        pdu.pdu_2d_pos_y = patchUVG.omPPPosY_;
        pdu.pdu_2d_size_x_minus1 = patchUVG.widthInPPBlk_ - 1;
        pdu.pdu_2d_size_y_minus1 = patchUVG.heightInPPBlk_ - 1;
        pdu.pdu_3d_offset_u = patchUVG.posU_;
        pdu.pdu_3d_offset_v = patchUVG.posV_;
        const size_t min_level = static_cast<size_t>(pow(2., ath.ath_pos_min_d_quantizer));
        pdu.pdu_3d_offset_d = (patchUVG.posD_ / min_level);
        pdu.pdu_3d_range_d = patchUVG.sizeD_ == 0 ? 0 : ((patchUVG.sizeD_ + 1) / min_level);
        pdu.pdu_projection_id = patchUVG.patchPpi_;
        pdu.pdu_orientation_index = static_cast<size_t>(patchUVG.axisSwap_);
        if (afps_.afps_lod_mode_enabled_flag) {
            pdu.pdu_lod_enabled_flag = (level_of_detail_x > 1 || level_of_detail_y > 1);
            if (pdu.pdu_lod_enabled_flag) {
                pdu.pdu_lod_scale_x_minus1 = 0;
                pdu.pdu_lod_scale_y_idc = 0;
            }
        }
        atdu.patch_information_data_.push_back(pid);
    }

    patch_information_data end_patch;
    end_patch.patchMode = APM::I_END;
    atdu.patch_information_data_.push_back(end_patch);
    return atdu;
}

atlas_tile_layer_rbsp atlas_context::create_atlas_tile_layer_rbsp(size_t frameIndex, size_t tileIndex,
                                                                  const std::shared_ptr<uvgvpcc_enc::FrameContext>& frameUVG) {
    atlas_tile_layer_rbsp rbsp;
    rbsp.ath_ = create_atlas_tile_header(frameIndex, tileIndex);
    rbsp.atdu_ = create_atlas_tile_data_unit(frameUVG, rbsp.ath_);
    return rbsp;
}

atlas_frame_tile_information atlas_context::create_atlas_frame_tile_information() const {
    const size_t num_partitions_in_atlas_frame = 1;
    atlas_frame_tile_information afti;
    afti.afti_single_tile_in_atlas_frame_flag = true;
    if (!afti.afti_single_tile_in_atlas_frame_flag) {
        afti.afti_uniform_partition_spacing_flag = false;
        if (!afti.afti_uniform_partition_spacing_flag) {
            afti.afti_num_partition_columns_minus1 = 0;
            afti.afti_num_partition_rows_minus1 = 0;
        }
        afti.afti_single_partition_per_tile_flag = false;
        if (!afti.afti_single_partition_per_tile_flag) {
            afti.afti_num_tiles_in_atlas_frame_minus1 = 0;
        } else {
            afti.afti_num_tiles_in_atlas_frame_minus1 = num_partitions_in_atlas_frame - 1;
        }
    } else {
        afti.afti_num_tiles_in_atlas_frame_minus1 = 0;
        afti.afti_single_partition_per_tile_flag = false;
        afti.afti_uniform_partition_spacing_flag = false;
    }
    if (asps_.asps_auxiliary_video_enabled_flag) {
        afti.afti_auxiliary_video_tile_row_width_minus1 = 0;
        afti.afti_auxiliary_video_tile_row_height.resize(afti.afti_num_tiles_in_atlas_frame_minus1 + 1, 0);
    }
    afti.afti_signalled_tile_id_flag = false;
    if (afti.afti_signalled_tile_id_flag) {
        afti.afti_signalled_tile_id_length_minus1 = 0;
        afti.afti_tile_id.resize(afti.afti_num_tiles_in_atlas_frame_minus1 + 1, 0);
    }
    return afti;
}

atlas_frame_parameter_set atlas_context::create_atlas_frame_parameter_set() {
    atlas_frame_parameter_set afps;
    afps.afps_atlas_frame_parameter_set_id = 0;
    afps.afps_atlas_sequence_parameter_set_id = 0;
    afps.afti = create_atlas_frame_tile_information();
    afps.afps_output_flag_present_flag = false;
    afps.afps_num_ref_idx_default_active_minus1 = 0;
    afps.afps_additional_lt_afoc_lsb_len = 0;
    afps.afps_lod_mode_enabled_flag = false;
    afps.afps_raw_3d_offset_bit_count_explicit_mode_flag = false;
    afps.afps_extension_present_flag = true;
    afps.afps_miv_extension_present_flag = false;
    afps.afps_extension_7bits = 0;
    return afps;
}

atlas_sequence_parameter_set atlas_context::create_atlas_sequence_parameter_set(const std::shared_ptr<uvgvpcc_enc::GOF>& gofUVG) {
    atlas_sequence_parameter_set asps;
    asps.asps_atlas_sequence_parameter_set_id = 0;
    asps.asps_frame_width = uvgvpcc_enc::p_->mapWidth;
    asps.asps_frame_height = gofUVG->mapHeightGOF;
    asps.asps_geometry_3d_bit_depth_minus1 = uvgvpcc_enc::p_->geoBitDepthInput;
    const size_t geometry_nominal_2d_bitdepth = 8;
    asps.asps_geometry_2d_bit_depth_minus1 = geometry_nominal_2d_bitdepth - 1;
    asps.asps_log2_max_atlas_frame_order_cnt_lsb_minus4 = 10 - 4;
    asps.asps_max_dec_atlas_frame_buffering_minus1 = 0;
    asps.asps_long_term_ref_atlas_frames_flag = false;
    asps.asps_num_ref_atlas_frame_lists_in_asps = 1;

    ref_list_struct refs;
    refs.num_ref_entries = 1;
    for (size_t i = 0; i < refs.num_ref_entries; ++i) {
        refs.st_ref_atlas_frame_flag.push_back(!asps.asps_long_term_ref_atlas_frames_flag);
        if (refs.st_ref_atlas_frame_flag.at(i)) {
            refs.abs_delta_afoc_st.push_back(1);
            if (refs.abs_delta_afoc_st.at(i) > 0) {
                refs.straf_entry_sign_flag.push_back(true);
            }
        }
    }
    asps.ref_lists.push_back(refs);

    asps.asps_use_eight_orientations_flag = false;
    asps.asps_extended_projection_enabled_flag = false;
    asps.asps_normal_axis_limits_quantization_enabled_flag = true;
    asps.asps_normal_axis_max_delta_value_enabled_flag = true;
    asps.asps_patch_precedence_order_flag = false;
    // asps.asps_log2_patch_packing_block_size = static_cast<uint8_t>(std::log2(uvgvpcc_enc::p_->occupancyMapDSResolution));
    asps.asps_log2_patch_packing_block_size = static_cast<uint8_t>(std::log2(uvgvpcc_enc::p_->patchPackingBlockSize));
    asps.asps_patch_size_quantizer_present_flag = false;
    asps.asps_map_count_minus1 = uvgvpcc_enc::p_->doubleLayer ? 1 : 0;
    asps.asps_pixel_deinterleaving_enabled_flag = false;
    asps.asps_raw_patch_enabled_flag = false;
    asps.asps_eom_patch_enabled_flag = false;
    asps.asps_plr_enabled_flag = false;
    asps.asps_vui_parameters_present_flag = false;
    asps.asps_extension_present_flag = true;
    asps.asps_vpcc_extension_present_flag = true;
    asps.asps_miv_extension_present_flag = false;
    asps.asps_extension_6bits = 0;
    asps.asps_vpcc_remove_duplicate_point_enabled_flag = true;
    return asps;
}

void atlas_context::initialize_atlas_context(const std::shared_ptr<uvgvpcc_enc::GOF>& gofUVG) {
    gof_id_ = gofUVG->gofId;
    asps_ = create_atlas_sequence_parameter_set(gofUVG);
    afps_ = create_atlas_frame_parameter_set();
    for (size_t frame_index = 0; frame_index < gofUVG->nbFrames; ++frame_index) {
        auto& frameUVG = gofUVG->frames[frame_index];
        const size_t tile_index = 0;
        atlas_data_.push_back(create_atlas_tile_layer_rbsp(frame_index, tile_index, frameUVG));
    }
    prepareAtlasContext(*this);
}
