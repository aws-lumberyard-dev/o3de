/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>

namespace AZ::RPI
{
    //! Custom data for the VRSImageGen Pass.
    struct VRSImageGenPassData
        : public RPI::ComputePassData
    {
        AZ_RTTI(VRSImageGenPassData, "{A57E0994-20BE-412A-B6C1-51419B945FDA}", ComputePassData);
        AZ_CLASS_ALLOCATOR(VRSImageGenPassData, SystemAllocator, 0);

        VRSImageGenPassData() = default;
        virtual ~VRSImageGenPassData() = default;

        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<VRSImageGenPassData, RPI::ComputePassData>()
                    ->Version(0)
                    ;
            }
        }

        
    };

    class VRSImageGenPass : public RPI::ComputePass
    {
        using Base = RPI::ComputePass;
        AZ_RPI_PASS(VRSImageGenPass);
        
    public:
        AZ_RTTI(AZ::Render::VRSImageGenPass, "{DC46D7DD-DD47-4C49-946B-5F51E36A0397}", Base);
        AZ_CLASS_ALLOCATOR(VRSImageGenPass, SystemAllocator, 0);
        virtual ~VRSImageGenPass() = default;
        
        /// Creates a VRSImageGenPass
        static RPI::Ptr<VRSImageGenPass> Create(const RPI::PassDescriptor& descriptor);
        float m_VarianceCutoff = 0.05f;
        float m_MotionFactor = 0.05f;
    private:

        VRSImageGenPass(const RPI::PassDescriptor& descriptor);
        
        // Scope producer functions...
        void CompileResources(const RHI::FrameGraphCompileContext& context) override;
        
        // Pass behavior overrides...
        void FrameBeginInternal(FramePrepareParams params) override;
        void ResetInternal() override;
        void BuildInternal() override;

      
        

        RHI::ShaderInputNameIndex g_VarianceCutoff = "g_VarianceCutoff";
        RHI::ShaderInputNameIndex g_MotionFactor = "g_MotionFactor";
       
        
    
    };

} // namespace AZ::Render
