/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Passes/OceanInitialSpectrumComputePass.h>
#include <OceanFeatureProcessor.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>

#include <random>

namespace Ocean
{
    void OceanInitialSpectrumComputePassData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Ocean::OceanInitialSpectrumComputePassData, AZ::RPI::ComputePassData>()
                ->Version(1);
        }
    }

    AZ::RPI::Ptr<OceanInitialSpectrumComputePass> OceanInitialSpectrumComputePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<OceanInitialSpectrumComputePass> pass = aznew OceanInitialSpectrumComputePass(descriptor);
        return pass;
    }

    OceanInitialSpectrumComputePass::OceanInitialSpectrumComputePass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
        const OceanInitialSpectrumComputePass* passData = AZ::RPI::PassUtils::GetPassData<OceanInitialSpectrumComputePass>(descriptor);
        if (passData)
        {
            // Copy data to pass
        }

    }

    void OceanInitialSpectrumComputePass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        ComputePass::BuildCommandListInternal(context);
    }

    void OceanInitialSpectrumComputePass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        UpdateSrg();
        ComputePass::CompileResources(context);
    }

    // See https://wikiwaves.org/Ocean-Wave_Spectra
    float OceanInitialSpectrumComputePass::JonswapAlpha(float gravity, float fetch, float windSpeed)
    {
        return 0.076f * std::powf((windSpeed * windSpeed) / (fetch * gravity), 0.22f);
    }

    // See https://wikiwaves.org/Ocean-Wave_Spectra
    float OceanInitialSpectrumComputePass::JonswapPeakFrequency(float gravity, float fetch, float windSpeed)
    {
        return 22.0f * std::powf((gravity * gravity) / (windSpeed * fetch), 1.0f / 3.0f);
    }

    auto OceanInitialSpectrumComputePass::GenerateShaderSpectrumSettings(const ExternalSpectrumSettings& settings) -> ShaderSpectrumSettings
    {
        ShaderSpectrumSettings shaderSettings;
        shaderSettings.m_scale = settings.m_scale;
        shaderSettings.m_angle = AZ::DegToRad(settings.m_windDirection);
        shaderSettings.m_spreadBlend = settings.m_spreadBlend;
        shaderSettings.m_swell = AZ::GetClamp(settings.m_swell, 0.01f, 1.0f);
        shaderSettings.m_alpha = JonswapAlpha(m_worldSettings.m_gravity, settings.m_fetch, settings.m_windSpeed);
        shaderSettings.m_peakOmega = JonswapPeakFrequency(m_worldSettings.m_gravity, settings.m_fetch, settings.m_windSpeed);
        shaderSettings.m_gamma = settings.m_peakEnhancement;
        shaderSettings.m_shortWavesFade = settings.m_shortWavesFade;
        return shaderSettings;
    }

    void OceanInitialSpectrumComputePass::UpdateSrg()
    {
        if (!m_gaussianDistributionImage)
        {
            // Generate normal distribution texture (this should be moved to an asset, possibly auto-generate and save if it doesn't exist)
            std::mt19937 gen;
            std::normal_distribution<float> distribution;

            const uint32_t patchDim = 256;
            uint32_t pixelCount = patchDim * patchDim;
            AZStd::vector<AZStd::array<float, 2>> pixels(pixelCount);
            for (uint32_t i = 0; i < pixelCount; ++i)
            {
                pixels.at(i).at(0) = distribution(gen);
                pixels.at(i).at(1) = distribution(gen);
            }

            AZ::Data::Instance<AZ::RPI::StreamingImagePool> streamingImagePool = AZ::RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();

            m_gaussianDistributionImage = AZ::RPI::StreamingImage::CreateFromCpuData(
                *streamingImagePool,
                AZ::RHI::ImageDimension::Image2D,
                AZ::RHI::Size(patchDim, patchDim, 1),
                AZ::RHI::Format::R32G32_FLOAT,
                pixels.data(),
                pixels.size() * sizeof(decltype(pixels)::value_type)
            );
        }

        m_shaderResourceGroup->SetImage(m_gaussianDistributionIndex, m_gaussianDistributionImage);

        ShaderConstants constants;

        OceanFeatureProcessor* fp = GetScene()->GetFeatureProcessor<OceanFeatureProcessor>();
        if (fp)
        {
            OceanFeatureProcessor::OceanSettings oceanSettings = fp->GetOceanSettings();
            constants.m_textureSize = oceanSettings.m_textureSize;
            constants.m_gravity = oceanSettings.m_gravity;
            constants.m_depth = oceanSettings.m_depth;
        }

        constants.m_lengthScale = 250.0f; // The scale of the area covered by this cascade (higher scales for smaller areas)

        // These control the range of frequencies covered by this cascade
        constants.m_cutoffLow = 0.00001f;
        constants.m_cutoffHigh = 2.2175f;

        m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);

        // Spectrum settings
        m_localSpectrumSettings.m_windDirection = 30.0f;

        m_swellSpectrumSettings.m_scale = 0.5f;
        m_swellSpectrumSettings.m_windSpeed = 1.0f;
        m_swellSpectrumSettings.m_fetch = 300'000.0f;
        m_swellSpectrumSettings.m_swell = 1.0f;

        m_shaderResourceGroup->SetConstant(m_localSpectrumSettingsIndex, GenerateShaderSpectrumSettings(m_localSpectrumSettings));
        m_shaderResourceGroup->SetConstant(m_swellSpectrumSettingsIndex, GenerateShaderSpectrumSettings(m_swellSpectrumSettings));
    }

} // namespace Terrain
