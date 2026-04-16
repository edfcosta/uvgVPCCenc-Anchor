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

/// \file Library parameters related operations.

#include "parameters.hpp"

#include <iterator>
#include <numeric>
#include <string>
#include <unordered_map>

#include "uvgutils/log.hpp"

namespace uvgvpcc_enc {

namespace {

uvgutils::ParameterMap parameterMap;

}  // anonymous namespace

void initializeParameterMap(Parameters& param) {
    using uvgutils::BOOL;
    using uvgutils::DOUBLE;
    using uvgutils::FLOAT;
    using uvgutils::INT;
    using uvgutils::STRING;
    using uvgutils::UINT;
    parameterMap = {
        // ___ General parameters __ //
        {"geoBitDepthInput", {UINT, "", &param.geoBitDepthInput}},
        {"presetName", {STRING, "fast,slow", &param.presetName}},
        {"intermediateFilesDir", {STRING, "", &param.intermediateFilesDir}},
        {"statisticsDir", {STRING, "", &param.statisticsDir}},
        {"sizeGOF", {UINT, "", &param.sizeGOF}},  // TODO(lf)merge both gof size param ?
        {"nbThreadPCPart", {UINT, "", &param.nbThreadPCPart}},
        {"maxConcurrentFrames", {UINT, "", &param.maxConcurrentFrames}},
        {"doubleLayer", {BOOL, "", &param.doubleLayer}},
        {"logLevel",
         {STRING,
          std::accumulate(std::next(std::begin(uvgutils::LogLevelStr)), std::end(uvgutils::LogLevelStr), uvgutils::LogLevelStr[0],
                          [](const std::string& a, const std::string& b) { return a + "," + b; }),
          &param.logLevel}},
        {"errorsAreFatal", {BOOL, "", &param.errorsAreFatal}},

        // ___ Debug parameters ___ //
        {"exportIntermediateFiles", {BOOL, "", &param.exportIntermediateFiles}},
        {"exportStatistics", {BOOL, "", &param.exportStatistics}},
        {"intermediateFilesDirTimeStamp", {BOOL, "", &param.intermediateFilesDirTimeStamp}},
        {"timerLog", {BOOL, "", &param.timerLog}},

        // ___ Activate or not some features ___ //
        {"lowDelayBitstream", {BOOL, "", &param.lowDelayBitstream}},

        // ___ Voxelization ___ //       (grid-based segmentation)
        {"geoBitDepthVoxelized", {UINT, "", &param.geoBitDepthVoxelized}},

        // ___ Slicing Algorithm ___ //
        {"activateSlicing", {BOOL, "", &param.activateSlicing}},

        // ___ KdTree ___ //
        {"kdTreeMaxLeafSize", {UINT, "", &param.kdTreeMaxLeafSize}},

        // Normal computation //
        {"normalComputationKnnCount", {UINT, "", &param.normalComputationKnnCount}},
        {"normalComputationMaxDiagonalStep", {UINT, "", &param.normalComputationMaxDiagonalStep}},

        // Normal orientation //
        {"normalOrientationKnnCount", {UINT, "", &param.normalOrientationKnnCount}},

        // PPI segmentation //

        // ___ PPI smoothing  ___  //    (fast grid-based refine segmentation)
        {"geoBitDepthRefineSegmentation", {UINT, "", &param.geoBitDepthRefineSegmentation}},
        {"refineSegmentationMaxNNVoxelDistanceLUT", {UINT, "", &param.refineSegmentationMaxNNVoxelDistanceLUT}},
        {"refineSegmentationLambda", {DOUBLE, "", &param.refineSegmentationLambda}},
        {"refineSegmentationIterationCount", {UINT, "", &param.refineSegmentationIterationCount}},
        {"refineSegmentationIDEVDist", {UINT, "", &param.refineSegmentationIDEVDist}},
        // __ PPI smoothing with Slicing Algorithm __ //
        {"slicingRefineSegmentationMaxNNVoxelDistanceLUT", {UINT, "", &param.slicingRefineSegmentationMaxNNVoxelDistanceLUT}},
        {"slicingRefineSegmentationLambda", {DOUBLE, "", &param.slicingRefineSegmentationLambda}},
        {"slicingRefineSegmentationIterationCount", {UINT, "", &param.slicingRefineSegmentationIterationCount}},
        {"slicingRefineSegmentationIDEVDist", {UINT, "", &param.slicingRefineSegmentationIDEVDist}},

        // ___ Patch generation ___ //   (patch segmentation)
        {"maxAllowedDist2RawPointsDetection", {UINT, "", &param.maxAllowedDist2RawPointsDetection}},
        {"minPointCountPerCC", {UINT, "", &param.minPointCountPerCC}},
        {"patchSegmentationMaxPropagationDistance", {UINT, "", &param.patchSegmentationMaxPropagationDistance}},
        {"enablePatchSplitting", {BOOL, "", &param.enablePatchSplitting}},
        {"minLevel", {UINT, "", &param.minLevel}},
        {"log2QuantizerSizeX", {UINT, "", &param.log2QuantizerSizeX}},
        {"log2QuantizerSizeY", {UINT, "", &param.log2QuantizerSizeY}},
        {"quantizerSizeX", {UINT, "", &param.quantizerSizeX}},
        {"quantizerSizeY", {UINT, "", &param.quantizerSizeY}},
        {"surfaceThickness", {UINT, "", &param.surfaceThickness}},

        // ___ Patch packing ___ //
        {"mapWidth", {UINT, "", &param.mapWidth}},
        {"minimumMapHeight", {UINT, "", &param.minimumMapHeight}},
        {"spacePatchPacking", {UINT, "", &param.spacePatchPacking}},
        {"interPatchPacking", {BOOL, "", &param.interPatchPacking}},
        {"gpaTresholdIoU", {FLOAT, "", &param.gpaTresholdIoU}},

        // ___ Map generation ___ //
        {"mapGenerationBackgroundValueAttribute", {UINT, "", &param.mapGenerationBackgroundValueAttribute}},
        {"mapGenerationBackgroundValueGeometry", {UINT, "", &param.mapGenerationBackgroundValueGeometry}},
        {"attributeBgFill", {STRING, "none,patchExtension,bbpe,pushPull", &param.attributeBgFill}},
        {"blockSizeBBPE", {UINT, "0,1,2,4,8,16,32,64,128", &param.blockSizeBBPE}},
        {"useTmc2YuvDownscaling", {BOOL, "", &param.useTmc2YuvDownscaling}},
        {"mapGenerationFillEmptyBlock", {BOOL, "", &param.mapGenerationFillEmptyBlock}},
        {"dynamicMapHeight", {BOOL, "", &param.dynamicMapHeight}},



        // ___ 2D encoding parameters ___ //
        {"sizeGOP2DEncoding", {UINT, "8,16", &param.sizeGOP2DEncoding}},
        {"intraFramePeriod", {UINT, "", &param.intraFramePeriod}},
        {"encoderInfoSEI", {BOOL, "", &param.encoderInfoSEI}},

// Occupancy map
#if LINK_FFMPEG
        {"occupancyEncoderName", {STRING, "Kvazaar,FFmpeg", &param.occupancyEncoderName}},
#else
        {"occupancyEncoderName", {STRING, "Kvazaar", &param.occupancyEncoderName}},
#endif
        {"occupancyEncodingIsLossless", {BOOL, "", &param.occupancyEncodingIsLossless}},
        {"occupancyEncodingMode", {STRING, "AI,RA", &param.occupancyEncodingMode}},
        {"occupancyEncodingFormat", {STRING, "YUV420", &param.occupancyEncodingFormat}},
        {"occupancyEncodingNbThread", {UINT, "", &param.occupancyEncodingNbThread}},
        {"occupancyMapDSResolution", {UINT, "2,4", &param.occupancyMapDSResolution}},
        {"occupancyEncodingPreset",
         {STRING, "ultrafast,superfast,veryfast,faster,fast,medium,slow,slower,veryslow", &param.occupancyEncodingPreset}},
        {"omRefinementTreshold2", {UINT, "1,2,3,4", &param.omRefinementTreshold2}},
        {"omRefinementTreshold4", {UINT, "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16", &param.omRefinementTreshold4}},
#if LINK_FFMPEG
        {"occupancyFFmpegCodecName", {STRING, "", &param.occupancyFFmpegCodecName}},
        {"occupancyFFmpegCodecOptions", {STRING, "", &param.occupancyFFmpegCodecOptions}},
        {"occupancyFFmpegCodecParams", {STRING, "", &param.occupancyFFmpegCodecParams}},
#endif

// Geometry map
#if LINK_FFMPEG
        {"geometryEncoderName", {STRING, "Kvazaar,FFmpeg", &param.geometryEncoderName}},
#else
        {"geometryEncoderName", {STRING, "Kvazaar", &param.geometryEncoderName}},
#endif
        {"geometryEncodingIsLossless", {BOOL, "", &param.geometryEncodingIsLossless}},
        {"geometryEncodingMode", {STRING, "AI,RA", &param.geometryEncodingMode}},
        {"geometryEncodingFormat", {STRING, "YUV420", &param.geometryEncodingFormat}},
        {"geometryEncodingNbThread", {UINT, "", &param.geometryEncodingNbThread}},
        {"geometryEncodingQp", {UINT, "", &param.geometryEncodingQp}},
        {"geometryEncodingPreset",
         {STRING, "ultrafast,superfast,veryfast,faster,fast,medium,slow,slower,veryslow", &param.geometryEncodingPreset}},
#if LINK_FFMPEG
        {"geometryFFmpegCodecName", {STRING, "", &param.geometryFFmpegCodecName}},
        {"geometryFFmpegCodecOptions", {STRING, "", &param.geometryFFmpegCodecOptions}},
        {"geometryFFmpegCodecParams", {STRING, "", &param.geometryFFmpegCodecParams}},
#endif

// Attribute map
#if LINK_FFMPEG
        {"attributeEncoderName", {STRING, "Kvazaar,FFmpeg", &param.attributeEncoderName}},
#else
        {"attributeEncoderName", {STRING, "Kvazaar", &param.attributeEncoderName}},
#endif
        {"attributeEncodingIsLossless", {BOOL, "", &param.attributeEncodingIsLossless}},
        {"attributeEncodingMode", {STRING, "AI,RA", &param.attributeEncodingMode}},
        {"attributeEncodingFormat", {STRING, "YUV420", &param.attributeEncodingFormat}},
        {"attributeEncodingNbThread", {UINT, "", &param.attributeEncodingNbThread}},
        {"attributeEncodingQp", {UINT, "", &param.attributeEncodingQp}},
        {"attributeEncodingPreset",
         {STRING, "ultrafast,superfast,veryfast,faster,fast,medium,slow,slower,veryslow", &param.attributeEncodingPreset}},
#if LINK_FFMPEG
        {"attributeFFmpegCodecName", {STRING, "", &param.attributeFFmpegCodecName}},
        {"attributeFFmpegCodecOptions", {STRING, "", &param.attributeFFmpegCodecOptions}},
        {"attributeFFmpegCodecParams", {STRING, "", &param.attributeFFmpegCodecParams}},
#endif

        // ___ Bitstream generation ___ //
        {"displayBitstreamGenerationFps", {BOOL, "", &param.displayBitstreamGenerationFps}},

    };
}

void setParameterValue(const std::string& parameterName, const std::string& parameterValue, const bool& fromPreset) {
    uvgutils::setParameterValue(parameterMap, parameterName, parameterValue, fromPreset);
}

}  // namespace uvgvpcc_enc
