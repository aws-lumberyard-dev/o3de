/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI.Reflect/ConstantsLayout.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ComputePass.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>

namespace Ocean
{
    struct OceanInitialSpectrumComputePassData
        : public AZ::RPI::ComputePassData
    {
        AZ_RTTI(Ocean::OceanInitialSpectrumComputePassData, "{BC43CC58-0FA0-4096-A6DA-950BA58BE7E8}", AZ::RPI::ComputePassData);
        AZ_CLASS_ALLOCATOR(Ocean::OceanInitialSpectrumComputePassData, AZ::SystemAllocator, 0);

        OceanInitialSpectrumComputePassData() = default;
        virtual ~OceanInitialSpectrumComputePassData() = default;

        static void Reflect(AZ::ReflectContext* context);
    };

    class OceanInitialSpectrumComputePass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(OceanInitialSpectrumComputePass);

    public:
        AZ_RTTI(Ocean::OceanInitialSpectrumComputePass, "{C2FFA428-6D52-4E5D-85E6-63D2C6F9ADE8}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Ocean::OceanInitialSpectrumComputePass, AZ::SystemAllocator, 0);

        struct ShaderConstants
        {
            uint32_t m_textureSize;
            float m_lengthScale;
            float m_cutoffLow;
            float m_cutoffHigh;
            float m_gravity;
            float m_depth;
        };

        struct WorldSettings
        {
            float m_gravity = 9.81f;
            float m_depth = 500.0f; // Ocean depth in meters
        };

        struct ExternalSpectrumSettings
        {
            float m_scale = 1.0; // Scale of the waves [0..1]
            float m_windSpeed = 0.5; // Wind speed in m/s
            float m_windDirection = 0.0; // Wind direction in degrees
            float m_fetch = 100'000.0f; // Down wind distance from shore in meters
            float m_spreadBlend = 1.0; // ??
            float m_swell = 0.2f; // Related to how lined-up the waves are with the wind
            float m_peakEnhancement = 3.3; // ?? Maybe something to do with gershner waves?
            float m_shortWavesFade = 0.01; // ?? Some kind of dampener on waves. Not sure if 'short' means frequency or amplitude
        };

        virtual ~OceanInitialSpectrumComputePass() = default;

        static AZ::RPI::Ptr<OceanInitialSpectrumComputePass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:

        struct ShaderSpectrumSettings
        {
            float m_scale; // Scale of the waves [0..1]
            float m_angle; // Wind direction in radians
            float m_spreadBlend; // ?? [0..1]
            float m_swell; // ?? should be higher for swell spectrum [0..1]
            float m_alpha; // ?? Based on gravity, wind speed, and fetch
            float m_peakOmega; // peak wave frequency based on gravity, wind speed, and fetch
            float m_gamma; // ?? peak enhancement? maybe something to do with gershner waves?
            float m_shortWavesFade; // ?? Some kind of dampener on waves
        };

        WorldSettings m_worldSettings;
        ExternalSpectrumSettings m_localSpectrumSettings;
        ExternalSpectrumSettings m_swellSpectrumSettings;

        void UpdateSrg();

        OceanInitialSpectrumComputePass(const AZ::RPI::PassDescriptor& descriptor);
        void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

        float JonswapAlpha(float gravity, float fetch, float windSpeed);
        float JonswapPeakFrequency(float gravity, float fetch, float windSpeed);

        ShaderSpectrumSettings GenerateShaderSpectrumSettings(const ExternalSpectrumSettings& settings);

        AZ::Data::Instance<AZ::RPI::StreamingImage> m_gaussianDistributionImage;
        AZ::RHI::ShaderInputNameIndex m_gaussianDistributionIndex = "m_gaussianDistribution";
        AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
        AZ::RHI::ShaderInputNameIndex m_localSpectrumSettingsIndex = "m_localSpectrumSettings";
        AZ::RHI::ShaderInputNameIndex m_swellSpectrumSettingsIndex = "m_swellSpectrumSettings";
    };
} // namespace AZ::Render
