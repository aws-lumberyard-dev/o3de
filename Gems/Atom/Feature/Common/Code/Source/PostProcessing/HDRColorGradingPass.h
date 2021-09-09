/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <PostProcess/ColorGrading/HDRColorGradingSettings.h>

namespace AZ
{
    namespace Render
    {
        /**
         *  The color grading pass.
         */
        class HDRColorGradingPass
            : public AZ::RPI::FullscreenTrianglePass
            //TODO: , public PostProcessingShaderOptionBase
        {
        public:
            AZ_RTTI(HDRColorGradingPass, "{E68E31A1-DB24-4AFF-A029-456A8B74C03C}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(HDRColorGradingPass, SystemAllocator, 0);

            virtual ~HDRColorGradingPass() = default;

            //! Creates a ColorGradingPass
            static RPI::Ptr<HDRColorGradingPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            HDRColorGradingPass(const RPI::PassDescriptor& descriptor);
            
            //! Pass behavior overrides
            void InitializeInternal() override;
            void FrameBeginInternal(FramePrepareParams params) final;

        private:
            const HDRColorGradingSettings* GetHDRColorGradingSettings() const;
            void SetSrgConstants();

            // Scope producer functions...
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            //void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            RHI::ShaderInputNameIndex m_colorGradingExposureIndex = "m_colorGradingExposure";
            //RHI::ShaderInputNameIndex m_colorGradingContrastIndex = "m_colorGradingContrast";
            //RHI::ShaderInputNameIndex m_colorGradingHueShiftIndex = "m_colorGradingHueShift";
            //RHI::ShaderInputNameIndex m_colorGradingPreSaturationIndex = "m_colorGradingPreSaturation";
            //RHI::ShaderInputNameIndex m_colorFilterIntensityIndex = "m_colorFilterIntensity";
            //RHI::ShaderInputNameIndex m_colorFilterMultiplyIndex = "m_colorFilterMultiply";
            //RHI::ShaderInputNameIndex m_kelvinIndex = "m_kelvin";
            //RHI::ShaderInputNameIndex m_kelvinLumPreservationIndex = "m_kelvinLumPreservation";
            //RHI::ShaderInputNameIndex m_kelvinColorMixIndex = "m_kelvinColorMix";
            //RHI::ShaderInputNameIndex m_whiteBalanceIndex = "m_whiteBalance";
            //RHI::ShaderInputNameIndex m_whiteBalanceTintIndex = "m_whiteBalanceTint";
            //RHI::ShaderInputNameIndex m_splitToneBalanceIndex = "m_splitToneBalance";
            //RHI::ShaderInputNameIndex m_splitToneMixIndex = "m_splitToneMix";
            RHI::ShaderInputNameIndex m_colorGradingPostSaturationIndex = "m_colorGradingPostSaturation";
            // TODO: Add other indices
        };
    }   // namespace Render
}   // namespace AZ
