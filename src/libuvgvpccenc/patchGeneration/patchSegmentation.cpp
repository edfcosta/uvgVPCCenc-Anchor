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
 * list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * * Neither the name of Tampere University, ITU/ISO/IEC nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

/// \file Entry point for the patch segmentation process which create the frame patch list.

#include "patchGeneration.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <robin_hood.h>
#include "utils/constants.hpp"
#include "utils/fileExport.hpp"
#include "utils/parameters.hpp"
#include "utilsPatchGeneration.hpp"
#include "uvgutils/log.hpp"
#include "uvgutils/utils.hpp"
#include "utils/types.hpp"
#include "utils/statsCollector.hpp"

using namespace uvgvpcc_enc;

namespace {

struct ConnectedComponent {
    std::vector<size_t> points;
    typeGeometryInput minU{};
    typeGeometryInput minV{};
    typeGeometryInput maxU{};
    typeGeometryInput maxV{};
    size_t ppi;
    size_t tangentAxis;
    size_t bitangentAxis;
    explicit ConnectedComponent(const size_t& ppi) : ppi(ppi) {
        switch (ppi) {
            case 0:
                tangentAxis = 2;
                bitangentAxis = 1;
                break;
            case 1:
                tangentAxis = 2;
                bitangentAxis = 0;
                break;
            case 2:
                tangentAxis = 0;
                bitangentAxis = 1;
                break;
            case 3:
                tangentAxis = 2;
                bitangentAxis = 1;
                break;
            case 4:
                tangentAxis = 2;
                bitangentAxis = 0;
                break;
            case 5:
                tangentAxis = 0;
                bitangentAxis = 1;
                break;
            default:
                assert(false);
                break;
        }
        points.reserve(65536);  // TODO(lf): depends on heuristic for input geometry size
    }
};

template<typename keyType>
inline bool findNeighborSeed(const uvgutils::VectorN<typeGeometryInput, 3>& ptSeed,
                             const robin_hood::unordered_set<keyType>& resamplePointSetLocation1D) {

    const size_t adjacentRange = adjacentPointsSearchFlatOffsets[p_->maxAllowedDist2RawPointsDetection];
    const size_t gbd = p_->geoBitDepthInput;
    const size_t gbd2 = p_->geoBitDepthInput * 2;
    const int maxVal = (1U << gbd) - 1;
    
    for(size_t i = 0; i < adjacentRange; ++i) {
        const auto& shift = adjacentPointsSearchFlat[i];
        
        const int x = static_cast<int>(ptSeed[0]) + shift[0];
        const int y = static_cast<int>(ptSeed[1]) + shift[1];
        const int z = static_cast<int>(ptSeed[2]) + shift[2];

        if (x < 0 || x > maxVal || y < 0 || y > maxVal || z < 0 || z > maxVal) continue;
        
        const size_t adjLoc1D = location1DFromCoordinates<keyType>(x,y,z,gbd,gbd2);
        if (resamplePointSetLocation1D.contains(adjLoc1D)) {
            return true;
        }
    }
    return false;
}

template <typename keyType>
inline void createConnectedComponent(const std::shared_ptr<uvgvpcc_enc::FrameContext>& frame, const size_t& seedIndex,
                                     std::vector<bool>& pointIsInAPatch, ConnectedComponent& cc,
                                     robin_hood::unordered_map<keyType, size_t>& mapLocation1D,
                                     const uvgutils::VectorN<typeGeometryInput, 3>& ptSeed,
                                     std::vector<size_t>& fifo) {
    cc.points.push_back(seedIndex);
    pointIsInAPatch[seedIndex] = true;
    const size_t gbd = p_->geoBitDepthInput;
    const size_t gbd2 = p_->geoBitDepthInput * 2;    
    mapLocation1D.erase(location1DFromPoint<keyType>(ptSeed,gbd,gbd2));

    const size_t uAxis = cc.tangentAxis;    // 0, 1 or 2
    const size_t vAxis = cc.bitangentAxis;  // 0, 1 or 2
    cc.minU = ptSeed[uAxis];
    cc.minV = ptSeed[vAxis];
    cc.maxU = ptSeed[uAxis];
    cc.maxV = ptSeed[vAxis];
    
    fifo.clear();
    fifo.emplace_back(seedIndex);

    const size_t adjacentRange = adjacentPointsSearchFlatOffsets[p_->patchSegmentationMaxPropagationDistance];
    const int maxVal = (1U << gbd) - 1;
    size_t fifoReadIndex = 0;
    while (fifoReadIndex < fifo.size()) {
        const size_t idx = fifo[fifoReadIndex++];
        const auto& pt = frame->pointsGeometry[idx];

        for(size_t i = 0; i < adjacentRange; ++i) {
            const auto& shift = adjacentPointsSearchFlat[i];
            const int x = static_cast<int>(pt[0]) + shift[0];
            const int y = static_cast<int>(pt[1]) + shift[1];
            const int z = static_cast<int>(pt[2]) + shift[2];

            if (x < 0 || x > maxVal || y < 0 || y > maxVal || z < 0 || z > maxVal) continue;

            const keyType adjLoc1D = location1DFromCoordinates<keyType>(x,y,z,gbd,gbd2);
            auto it = mapLocation1D.find(adjLoc1D);
            if (it != mapLocation1D.end()) {
                const size_t adjPtIndex = it->second;
                mapLocation1D.erase(it);
                cc.points.push_back(adjPtIndex);
                pointIsInAPatch[adjPtIndex] = true;
                fifo.emplace_back(adjPtIndex);
                
                const uvgutils::VectorN<typeGeometryInput, 3> adjPt = {
                    static_cast<typeGeometryInput>(x),
                    static_cast<typeGeometryInput>(y),
                    static_cast<typeGeometryInput>(z)
                };
                const typeGeometryInput adjU = adjPt[uAxis];
                const typeGeometryInput adjV = adjPt[vAxis];

                cc.minU = std::min(cc.minU, adjU);
                cc.minV = std::min(cc.minV, adjV);
                cc.maxU = std::max(cc.maxU, adjU);
                cc.maxV = std::max(cc.maxV, adjV);
            }
        }
    }
}

template <size_t Ppi>
constexpr size_t getPatchNormalAxis() {
    assert(Ppi < 6);
    if constexpr (Ppi == 0 || Ppi == 3) return 0;
    if constexpr (Ppi == 1 || Ppi == 4) return 1;
    return 2;
}

template <size_t Ppi>
constexpr size_t getPatchTangentAxis() {
    assert(Ppi < 6);
    if constexpr (Ppi == 2 || Ppi == 5) return 0;
    return 2;
}

template <size_t Ppi>
constexpr size_t getPatchBitangentAxis() {
    assert(Ppi < 6);
    if constexpr (Ppi == 1 || Ppi == 4) return 0;
    return 1;
}

template <size_t Ppi>
constexpr bool getPatchProjectionMode() {
    assert(Ppi < 6);
    if constexpr (Ppi == 0 || Ppi == 1 || Ppi == 2) return 0;
    return 1;
}

template <size_t NormalAxis, size_t TangentAxis, size_t BitangentAxis, bool ProjectionMode>
inline void setInitialPatchL1(Patch& patch, const ConnectedComponent& cc, std::vector<typeGeometryInput>& peakPerBlock,
                              const std::shared_ptr<uvgvpcc_enc::FrameContext>& frame, int& extremum_D) {
    const size_t widthInPixel = patch.widthInPixel_;
    const size_t occRes = p_->occupancyMapDSResolution;  // TODO(lf) create an associated log parameter for occupancyMapDSResolution
    const size_t ppbRes = occRes == 1 ? 2 : occRes; // peak per block filter block size in pixel (resolution)
    const size_t widthInPpBlk = widthInPixel / ppbRes; // TODO(lf): should be ppblock

    for (const size_t pointIndex : cc.points) {
        const auto& point = frame->pointsGeometry[pointIndex];
        const typeGeometryInput d = static_cast<typeGeometryInput>(point[NormalAxis]);

        const size_t u = static_cast<size_t>(point[TangentAxis] - patch.posU_);
        const size_t v = static_cast<size_t>(point[BitangentAxis] - patch.posV_);
        const size_t p = v * widthInPixel + u;
        
        const size_t uPpBlk = u / ppbRes;
        const size_t vPpBlk = v / ppbRes;        
        const size_t pPpBlk = vPpBlk * widthInPpBlk + uPpBlk;

        assert(u < widthInPixel);
        assert(p < patch.depthL1_.size());
        const typeGeometryInput patchD = patch.depthL1_[p];

        if constexpr (!ProjectionMode) { // if 0: positive axis --> max search | if 1: negative axis --> min search
            assert(pPpBlk < peakPerBlock.size());
            peakPerBlock[pPpBlk] = std::max(peakPerBlock[pPpBlk], d);
            extremum_D = std::min(static_cast<int>(d), extremum_D); // Minimum value because decoder --> d - extremum_D
            if (patchD >= d && patchD != g_infiniteDepth) continue;
        } else {
            assert(pPpBlk < peakPerBlock.size());
            peakPerBlock[pPpBlk] = std::min(peakPerBlock[pPpBlk], d);
            extremum_D = std::max(static_cast<int>(d), extremum_D); // Maximum value because decoder --> extremum_D - d
            if (patchD <= d) continue;
        }

        //                                              extrD  --------------------> x axis
        // valid point for L1                             V                     v
        // lf : if 0, then L1 hold the highest  values    |       XX--X--------XX       highest  values
        // lf : if 1, then L1 hold the smallest values            XX--X--------XX   |   smallest values
        //                                                        ^                 ^
        //                                                                        extrD                 

        patch.depthL1_[p] = d;
        patch.depthPCidxL1_[p] = pointIndex;
    }
}

// Not used anymore
template <bool ProjectionMode>
inline int getMinD(const std::vector<typeGeometryInput>& peakPerBlock) {
    const size_t minLevel = p_->minLevel;
    if constexpr (ProjectionMode) {
        auto maxIt = std::max_element(peakPerBlock.begin(), peakPerBlock.end());
        const typeGeometryInput maxVal = (maxIt != peakPerBlock.end()) ? *maxIt : 0;
        return static_cast<int>(uvgutils::roundUp(maxVal, minLevel));
    } else {
        auto minIt = std::min_element(peakPerBlock.begin(), peakPerBlock.end());
        const typeGeometryInput minVal = (minIt != peakPerBlock.end()) ? *minIt : g_infiniteDepth;
        return static_cast<int>((minVal / minLevel) * minLevel);
    }
}

template <bool ProjectionMode>
inline void setPatchL1(Patch& patch, const int& extremum_D, const std::vector<typeGeometryInput>& peakPerBlock) {
    const size_t valueOverflowCheck =
        (1U << 8U) - 1 - p_->surfaceThickness;  // lf: In TMC2, 8 corresponds to geometryNominal2dBitdepth, which probably refers to the
                                                // geometry ouput (geometry maps use uint8)
    const size_t occRes = p_->occupancyMapDSResolution;
    const size_t ppbRes = occRes == 1 ? 2 : occRes; // peak per block filter block size in pixel (resolution)
    const size_t widthInPpBlk = patch.widthInPixel_ / ppbRes;

    for (size_t v = 0; v < patch.heightInPixel_; ++v) {
        for (size_t u = 0; u < patch.widthInPixel_; ++u) {
            const size_t pos = v * patch.widthInPixel_ + u;
            const typeGeometryInput depth = patch.depthL1_[pos];
            if (depth == g_infiniteDepth) {
                continue;
            }

            // check if the current depth value is small enough to be stored in the geometry map (uint8)
            const bool overflow = ProjectionMode ? extremum_D > valueOverflowCheck + depth : depth > valueOverflowCheck + extremum_D;
            if (overflow) {
                patch.depthL1_[pos] = g_infiniteDepth;
                patch.depthPCidxL1_[pos] = g_infinitenumber;
                continue;
            }

            const size_t uPpBlk = u / ppbRes;
            const size_t vPpBlk = v / ppbRes;        
            const size_t pPpBlk = vPpBlk * widthInPpBlk + uPpBlk;

            assert(pPpBlk < peakPerBlock.size());
            const int tmp_a = std::abs(depth - peakPerBlock[pPpBlk]);

            // If there is a hole (a missing point) in a patch, and it happens that this patch is long and overlap itself, then this check
            // allows not to put the isolated point.
            if (tmp_a > p_->distanceFiltering) {
                patch.depthL1_[pos] = g_infiniteDepth;
                patch.depthPCidxL1_[pos] = g_infinitenumber;
                // The lowest (minimum depth) point at this position amoung all the points in the patch being at this position (1 or more) is
                // too far away (>32). So, all the remaining points at this position, if they exist, are higher than the lowest points, and so
                // they are also further away than 32.
                continue;
            }

            patch.patchOccupancyMap_[pos] = 1;
            if(p_->exportIntermediateFiles) {
                patch.patchOccupancyMapColor_[pos] = patch.patchIndex_;
            }

            if constexpr (ProjectionMode) {
                patch.depthL1_[pos] = static_cast<int16_t>((static_cast<int16_t>(extremum_D) - patch.depthL1_[pos])); // Turn into relative position
                //                                    axis --------------->     extrD 
                //                                          XX-X-------XX         |
                //                                      d = ^~~~~~~~~~~~~~~~~~~~~~^ 
            } else {
                patch.depthL1_[pos] = static_cast<int16_t>(patch.depthL1_[pos] - static_cast<int16_t>(extremum_D)); // Turn into relative position
                //                                 extrD   ---------------> axis     
                //                                   |      XX-X-------XX         
                //                               d = ^~~~~~~~~~~~~~~~~~~^ 
            }
        }
    }
}

template <typename keyType, size_t Ppi, bool DoubleLayer>
inline void finalizePatch(const ConnectedComponent& cc, const std::shared_ptr<uvgvpcc_enc::FrameContext>& frame, Patch& patch,
                          robin_hood::unordered_map<keyType, size_t>& mapLocation1D, std::vector<bool>& pointIsInAPatch,
                          const typeGeometryInput& extremum_D, robin_hood::unordered_set<keyType>& resamplePointSetLocation1D) {
    constexpr size_t normalAxis = getPatchNormalAxis<Ppi>();
    constexpr size_t tangentAxis = getPatchTangentAxis<Ppi>();
    constexpr size_t bitangentAxis = getPatchBitangentAxis<Ppi>();
    constexpr bool projectionMode = getPatchProjectionMode<Ppi>();

    patch.sizeD_ = 0;

    if constexpr (DoubleLayer) {
        patch.depthL2_ = patch.depthL1_;            // Deep copy
        patch.depthPCidxL2_ = patch.depthPCidxL1_;  // Deep copy
    }

    const size_t gbd = p_->geoBitDepthInput;
    const size_t gbd2 = p_->geoBitDepthInput * 2;

    for (const size_t& pointIndex : cc.points) {
        const auto& point = frame->pointsGeometry[pointIndex];
        const size_t u = static_cast<size_t>(point[tangentAxis] - patch.posU_);
        const size_t v = static_cast<size_t>(point[bitangentAxis] - patch.posV_);
        const size_t p = v * patch.widthInPixel_ + u;
        const typeGeometryInput patchDL1 = patch.depthL1_[p];
        const keyType loc1D = location1DFromPoint<keyType>(point,gbd,gbd2);

        if (patchDL1 == g_infiniteDepth) {
            pointIsInAPatch[pointIndex] = false;
            mapLocation1D.emplace(loc1D, pointIndex);
            // lf: this point has been filtered (tmp_a>32). It will be processed during next iteration. There is no point in L1 here as the
            // filtering process is done on block of pixels.
            continue;
        }

        patch.sizeD_ = std::max<size_t>(patch.sizeD_, static_cast<size_t>(patchDL1));

        const typeGeometryInput d = projectionMode ? (extremum_D - point[normalAxis]) : (point[normalAxis] - extremum_D);
        //        ---------------------> axis
        // If 0:   |   XX-X-------XX           " | " is extremum_D
        //     d = ^~~~~~~~~~~~~~~~^    = pt - extremum_D
        // If 1:       XX-X-------XX  |
        //        d =  ^~~~~~~~~~~~~~~^ = extremum_D - pt              

        if (patchDL1 == d) {
            // lf: this point is part of L1
            assert(patch.depthPCidxL1_[p] == pointIndex);
            resamplePointSetLocation1D.emplace(loc1D);
            continue;
        }

        if constexpr (DoubleLayer) {
            assert(d < patchDL1);
            assert(d != patch.depthL2_[p]);
            const typeGeometryInput deltaD = patchDL1 - d;      
                if (d > patch.depthL2_[p]) {
                // This point is between the two layers, it is discarded.
                //          ------------------> axis
                //                        L2  L1
                //                        v   v
                // If 0:   |   XX-X-------XX--X 
                //                         ^
                //  d_L1 = a / d_L2 = a-2 
                //  d = a-1 --> d > d_L2  
                //         
                //             L1 L2
                //             v  v
                // If 1:       XX-X-------XX--X  | 
                //              ^
                //  d_L1 = a / d_L2 = a-2 
                //  d = a-1 --> d > d_L2  
                //  
                continue;
            }
            // [1]
            if (deltaD <= p_->surfaceThickness) { // If the point is inside the surface thickness
                if (patch.depthL2_[p] != g_infiniteDepth && patch.depthL2_[p] != patchDL1) { // if the point is not part of the L1 [2]
                    const auto overwrittenIdx = patch.depthPCidxL2_[p];
                    const auto& overwrittenPt = frame->pointsGeometry[overwrittenIdx];
                    const keyType overwrittenLoc1D = location1DFromPoint<keyType>(overwrittenPt,gbd,gbd2);
                    resamplePointSetLocation1D.erase(overwrittenLoc1D);
                    // The overwritten point is between the two layers, it is discarded.
                    //        L1  L2   |       L1  L2    |       L1    L2
                    //  [1]:  v   v    |  [2]: v - v *   |  [3]: v     v
                    //        X - X    |       X - X X   |       X - - X
                }
                patch.depthL2_[p] = d; // [3]
                patch.depthPCidxL2_[p] = pointIndex; // [3]
                resamplePointSetLocation1D.emplace(loc1D);
                patch.sizeD_ = std::max<size_t>(patch.sizeD_, static_cast<size_t>(patch.depthL2_[p]));
                continue;
            }
            if (deltaD <= p_->maxAllowedDist2RawPointsDetection) {
                // Out of range: 32
                // Spiral case: inside a Connected Component, there is an overlapping
                continue;
            }
            pointIsInAPatch[pointIndex] = false;
            mapLocation1D.emplace(loc1D, pointIndex);
        } else {
            assert(d < patchDL1);
            const typeGeometryInput deltaD = patchDL1 - d;
            if (deltaD < p_->surfaceThickness) {
                continue;
            }
            pointIsInAPatch[pointIndex] = false;
            mapLocation1D.emplace(loc1D, pointIndex);
        }
    }
}

template <typename keyType,size_t Ppi>
inline void createPatch(Patch& patch, const ConnectedComponent& cc, const std::shared_ptr<uvgvpcc_enc::FrameContext>& frame,
                        std::vector<bool>& pointIsInAPatch, robin_hood::unordered_map<keyType, size_t>& mapLocation1D,
                        robin_hood::unordered_set<keyType>& resamplePointSetLocation1D,
                        std::vector<typeGeometryInput>& sharedPeakPerBlock) {
    uvgutils::Logger::log<uvgutils::LogLevel::TRACE>("PATCH GENERATION", "Create patch for frame " + std::to_string(frame->frameId) + "\n");
    constexpr size_t normalAxis = getPatchNormalAxis<Ppi>();
    constexpr size_t tangentAxis = getPatchTangentAxis<Ppi>();
    constexpr size_t bitangentAxis = getPatchBitangentAxis<Ppi>();
    constexpr bool projectionMode = getPatchProjectionMode<Ppi>();
    
    const size_t dsRes = p_->occupancyMapDSResolution; // TODO(lf) TODO(mf) One day, the peak per block filter will not be linked to occ ds but to something else (maybe constant). Temporary fix for om ds == 1 is to set this filter block size to 2x2.
    const size_t width = cc.maxU - cc.minU;
    const size_t height = cc.maxV - cc.minV;
    
    // const size_t ppbRes = dsRes == 1 ? 2 : dsRes; // peak per block filter block size in pixel (resolution)
    const size_t ppbRes = p_->peakPerBlockBlockSize; // peak per block filter block size in pixel (resolution)
    
    patch.normalAxis_ = normalAxis;
    patch.tangentAxis_ = tangentAxis;
    patch.bitangentAxis_ = bitangentAxis;
    patch.projectionMode_ = projectionMode;
    patch.patchPpi_ = Ppi;
    patch.posU_ = cc.minU;
    patch.posV_ = cc.minV;
    

    const size_t quantizerPatchSize = std::max(dsRes, std::max(ppbRes,p_->patchPackingBlockSize));

    patch.widthInPixel_ = uvgutils::roundUp(width + 1, quantizerPatchSize); // TODO(lf) do we need the plus 1 ?
    patch.heightInPixel_ = uvgutils::roundUp(height + 1, quantizerPatchSize);

    patch.widthInPPBlk_ = patch.widthInPixel_ / p_->patchPackingBlockSize; 
    patch.heightInPPBlk_ = patch.heightInPixel_ / p_->patchPackingBlockSize;
    
    const size_t widthPeakPerBlockBlk = patch.widthInPixel_ / ppbRes;
    const size_t heightPeakPerBlockBlk = patch.heightInPixel_ / ppbRes;
    
    const size_t patchSize = patch.widthInPixel_ * patch.heightInPixel_;
    patch.patchOccupancyMap_.assign(patchSize, 0);
    
    if(p_->exportIntermediateFiles) {
        patch.patchOccupancyMapColor_.assign(patchSize, 0);
    }
    
    patch.area_ = patchSize;    
    patch.depthL1_.assign(patchSize, g_infiniteDepth);
    patch.depthPCidxL1_.assign(patchSize, g_infinitenumber);
    sharedPeakPerBlock.assign(widthPeakPerBlockBlk * heightPeakPerBlockBlk, projectionMode ? g_infiniteDepth : 0);
        // If projectionMode 0: positive axis --> find max --> init to 0
        //                       ------------------> axis
        //                    min               
        //                     |    XX-X-------XX
        //                     ---------------->^
        // If projectionMode 1: negative axis --> find min --> init to infinite
        //                       ------------------> axis
        //                                           max
        //                          XX-X-------XX     |
        //                          ^<-----------------

    int extremum_D = projectionMode ? 0 : g_infiniteDepth; // Was minD but can be a max. So name is now "extremum_D"
    setInitialPatchL1<normalAxis, tangentAxis, bitangentAxis, projectionMode>(patch, cc, sharedPeakPerBlock, frame, extremum_D);

    extremum_D = projectionMode ? static_cast<int>(uvgutils::roundUp(extremum_D, p_->minLevel)) :  static_cast<int>((extremum_D/p_->minLevel)*p_->minLevel);
    patch.posD_ = static_cast<size_t>(extremum_D);

    setPatchL1<projectionMode>(patch, extremum_D, sharedPeakPerBlock);

    if (p_->doubleLayer) {
        finalizePatch<keyType, Ppi, true>(cc, frame, patch, mapLocation1D, pointIsInAPatch, extremum_D, resamplePointSetLocation1D);
    } else {
        finalizePatch<keyType, Ppi, false>(cc, frame, patch, mapLocation1D, pointIsInAPatch, extremum_D, resamplePointSetLocation1D);
    }
}

template <typename keyType, bool FirstIteration>
inline void createConnectedComponents(std::vector<bool>& pointIsInAPatch, std::vector<bool>& pointCanBeASeed,
                                      const std::shared_ptr<uvgvpcc_enc::FrameContext>& frame,
                                      const robin_hood::unordered_set<keyType>& resamplePointSetLocation1D,
                                      const std::vector<size_t>& pointsPPIs,
                                      std::array<robin_hood::unordered_map<keyType, size_t>, 6>& mapList,
                                      std::vector<ConnectedComponent>& connectedComponents,
                                      std::vector<size_t>& sharedFifo) {
    uvgutils::Logger::log<uvgutils::LogLevel::TRACE>("PATCH GENERATION",
                                                     "Create connected components for frame " + std::to_string(frame->frameId) + "\n");
    for (size_t seedIndex = 0; seedIndex < pointIsInAPatch.size(); ++seedIndex) {
        if (pointIsInAPatch[seedIndex]) continue;
        if constexpr (!FirstIteration) {
            if (!pointCanBeASeed[seedIndex]) continue;
        }

        const uvgutils::VectorN<typeGeometryInput, 3>& ptSeed = frame->pointsGeometry[seedIndex];
        if constexpr (!FirstIteration) {
            // Find a correct seed point to start a connected component
            if (findNeighborSeed(ptSeed, resamplePointSetLocation1D)) {
                pointCanBeASeed[seedIndex] = false;
                continue;
            }
        }

        // There is no neighboring point of this seed that is in the resample. It is then a correct seed.
        const size_t ppiCC = pointsPPIs[seedIndex];
        connectedComponents.emplace_back(ppiCC);
        createConnectedComponent<keyType>(frame, seedIndex, pointIsInAPatch, connectedComponents.back(), mapList[ppiCC], ptSeed, sharedFifo);
    }
}

}  // Anonymous namespace

template<typename keyType>
void PatchGeneration::patchSegmentation(const std::vector<size_t>& pointsPPIs) {
    uvgutils::Logger::log<uvgutils::LogLevel::TRACE>("PATCH GENERATION",
                                                     "Patch segmentation of frame " + std::to_string(frame_->frameId) + "\n");

    const size_t pointCount = frame_->pointsGeometry.size();
    
    auto gofPtr = frame_->gof.lock();
    assert(gofPtr);
    const size_t framePos = frame_->frameId % p_->sizeGOF;
    
    std::vector<bool> pointIsInAPatch(pointCount, false);
    std::vector<bool> pointCanBeASeed(pointCount, true);
    std::array<robin_hood::unordered_map<keyType, size_t>, 6> mapList;
    for (auto& map : mapList) {
        map.reserve(65536);
    }

    const size_t gbd = p_->geoBitDepthInput;
    const size_t gbd2 = p_->geoBitDepthInput * 2;    
    for (size_t ptIndex = 0; ptIndex < pointCount; ++ptIndex) {
        const keyType pointLocation1D = location1DFromPoint<keyType>(frame_->pointsGeometry[ptIndex],gbd,gbd2);
        assert(pointsPPIs[ptIndex] < 6);
        mapList[pointsPPIs[ptIndex]].emplace(pointLocation1D, ptIndex);
    }
    
    robin_hood::unordered_set<keyType> resamplePointSetLocation1D;
    resamplePointSetLocation1D.reserve(pointCount);
    
    std::vector<ConnectedComponent> connectedComponents;
    connectedComponents.reserve(256);

    std::vector<size_t> sharedFifo;
    sharedFifo.reserve(65536);
    std::vector<typeGeometryInput> sharedPeakPerBlock;
    sharedPeakPerBlock.reserve(16384); 
    
    // Connected components creation (first iteration)
    createConnectedComponents<keyType,true>(pointIsInAPatch, pointCanBeASeed, frame_, resamplePointSetLocation1D, pointsPPIs, mapList,
        connectedComponents, sharedFifo);
        
    
    auto& patchList = *frame_->patchList;
    patchList.reserve(256);
    
    while (!connectedComponents.empty()) {
        // Patches creation
        for (const ConnectedComponent& cc : connectedComponents) {
            if (cc.points.size() < p_->minPointCountPerCC) continue;
            patchList.emplace_back();
            auto& patch = patchList.back();
            patch.patchIndex_ = patchList.size();

            switch (cc.ppi) {
                case 0:
                    createPatch<keyType,0>(patch, cc, frame_, pointIsInAPatch, mapList[0], resamplePointSetLocation1D, sharedPeakPerBlock);
                    break;
                case 1:
                    createPatch<keyType,1>(patch, cc, frame_, pointIsInAPatch, mapList[1], resamplePointSetLocation1D, sharedPeakPerBlock);
                    break;
                case 2:
                    createPatch<keyType,2>(patch, cc, frame_, pointIsInAPatch, mapList[2], resamplePointSetLocation1D, sharedPeakPerBlock);
                    break;
                case 3:
                    createPatch<keyType,3>(patch, cc, frame_, pointIsInAPatch, mapList[3], resamplePointSetLocation1D, sharedPeakPerBlock);
                    break;
                case 4:
                    createPatch<keyType,4>(patch, cc, frame_, pointIsInAPatch, mapList[4], resamplePointSetLocation1D, sharedPeakPerBlock);
                    break;
                case 5:
                    createPatch<keyType,5>(patch, cc, frame_, pointIsInAPatch, mapList[5], resamplePointSetLocation1D, sharedPeakPerBlock);
                    break;
                default:
                    assert(false);
                    break;
            }
        }

        // Connected components creation
        connectedComponents.clear();
        createConnectedComponents<keyType,false>(pointIsInAPatch, pointCanBeASeed, frame_, resamplePointSetLocation1D, pointsPPIs,
                                         mapList, connectedComponents, sharedFifo);
    }

    if(p_->exportStatistics){
        size_t numberOfLostPointPS = 0;
        std::vector<uvgutils::VectorN<uint8_t, 3>> attributes(frame_->pointsGeometry.size());
        std::vector<bool> pointColored(frame_->pointsGeometry.size(), false);
        for (const auto& patch : patchList) {
            const auto color = patchColors[patch.patchIndex_ % patchColors.size()];
            for (size_t v = 0; v < patch.heightInPixel_; ++v) {
                for (size_t u = 0; u < patch.widthInPixel_; ++u) {
                    const size_t pos = v * patch.widthInPixel_ + u;
                    const typeGeometryInput depth = patch.depthL1_[pos];
                    if (depth == g_infiniteDepth) {
                        if (p_->doubleLayer) {
                            assert(patch.depthL2_[pos] == g_infiniteDepth);
                        }
                        continue;
                    }
                    const size_t ptIndexL1 = patch.depthPCidxL1_[pos];
                    attributes[ptIndexL1] = color;
                    pointColored[ptIndexL1] = true;

                    if (!p_->doubleLayer) continue;
                    const size_t ptIndexL2 = patch.depthPCidxL2_[pos];
                    if (ptIndexL1 == ptIndexL2) continue;
                    attributes[ptIndexL2] = color;
                    pointColored[ptIndexL2] = true;
                }
            }
        }
        for (int i = 0; i < frame_->pointsGeometry.size(); ++i) {
            if (pointColored[i]) continue;
            // Points that are not within a patch are colored in red
            numberOfLostPointPS++;
        }
        // stats.setNumberOfLostPoints(frame->frameId, numberOfLostPointPS);
        stats.collectData(frame_->frameId, DataId::NumberOfLostPoints, numberOfLostPointPS);
    }


    if (p_->exportIntermediateFiles) {
        FileExport::exportPointCloudPatchSegmentationColor(frame_);
        FileExport::exportPointCloudPatchSegmentationBorder(frame_);
        FileExport::exportPointCloudPatchSegmentationBorderBlank(frame_);
    }

}

template void PatchGeneration::patchSegmentation<uint64_t>(
    const std::vector<size_t>&
);


template void PatchGeneration::patchSegmentation<uint32_t>(
    const std::vector<size_t>&
);

template void PatchGeneration::patchSegmentation<uint16_t>(
    const std::vector<size_t>&
);