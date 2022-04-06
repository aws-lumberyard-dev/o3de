/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Passes/OceanConjugatedSpectrumComputePass.h>
#include <OceanFeatureProcessor.h>
#include <Passes/OceanInitialSpectrumComputePass.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>

#include <random>

namespace Ocean
{
    void OceanConjugatedSpectrumComputePassData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Ocean::OceanConjugatedSpectrumComputePassData, AZ::RPI::ComputePassData>()
                ->Version(1);
        }
    }

    AZ::RPI::Ptr<OceanConjugatedSpectrumComputePass> OceanConjugatedSpectrumComputePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<OceanConjugatedSpectrumComputePass> pass = aznew OceanConjugatedSpectrumComputePass(descriptor);
        return pass;
    }

    OceanConjugatedSpectrumComputePass::OceanConjugatedSpectrumComputePass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
        const OceanConjugatedSpectrumComputePass* passData = AZ::RPI::PassUtils::GetPassData<OceanConjugatedSpectrumComputePass>(descriptor);
        if (passData)
        {
            // Copy data to pass
        }
    }

    void OceanConjugatedSpectrumComputePass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        ComputePass::BuildCommandListInternal(context);
    }

    void OceanConjugatedSpectrumComputePass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        ComputePass::CompileResources(context);
    }

    void OceanConjugatedSpectrumComputePass::UpdateSrg()
    {
        OceanInitialSpectrumComputePass::ShaderConstants constants;

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

    }

} // namespace Terrain
