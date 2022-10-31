/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Pass/VRSImageGenPass.h>

#include <AzCore/Math/Random.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>

namespace AZ::RPI
{
    
    RPI::Ptr<VRSImageGenPass> VRSImageGenPass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<VRSImageGenPass> pass = aznew VRSImageGenPass(descriptor);
        return pass;
    }
    
    VRSImageGenPass::VRSImageGenPass(const RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {

    }

    void VRSImageGenPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        m_shaderResourceGroup->SetConstant(g_VarianceCutoff, m_VarianceCutoff);
        m_shaderResourceGroup->SetConstant(g_MotionFactor, m_MotionFactor);
        Base::CompileResources(context);
        //m_shaderResourceGroup->Compile();
    }
    
    void VRSImageGenPass::FrameBeginInternal(FramePrepareParams params)
    {
 
        Base::FrameBeginInternal(params);
    }
    
    void VRSImageGenPass::ResetInternal()
    {

        Base::ResetInternal();
    }

    void VRSImageGenPass::BuildInternal()
    {
      
        Base::BuildInternal();
    }

} // namespace AZ::Render
