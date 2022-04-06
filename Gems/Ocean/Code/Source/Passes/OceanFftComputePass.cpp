/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Passes/OceanFftComputePass.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>

#include <random>

namespace Ocean
{
    void OceanFftComputePassData::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Ocean::OceanFftComputePassData, AZ::RPI::ComputePassData>()
                ->Version(1);
        }
    }

    AZ::RPI::Ptr<OceanFftComputePass> OceanFftComputePass::Create(const AZ::RPI::PassDescriptor& descriptor)
    {
        AZ::RPI::Ptr<OceanFftComputePass> pass = aznew OceanFftComputePass(descriptor);
        return pass;
    }

    OceanFftComputePass::OceanFftComputePass(const AZ::RPI::PassDescriptor& descriptor)
        : AZ::RPI::ComputePass(descriptor)
    {
        const OceanFftComputePass* passData = AZ::RPI::PassUtils::GetPassData<OceanFftComputePass>(descriptor);
        if (passData)
        {
            // Copy data to pass
        }
    }

    void OceanFftComputePass::BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context)
    {
        ComputePass::BuildCommandListInternal(context);
    }

    void OceanFftComputePass::CompileResources(const AZ::RHI::FrameGraphCompileContext& context)
    {
        ComputePass::CompileResources(context);
    }


} // namespace Terrain
