/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Reflect/Pass/ComputePassData.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace Ocean
{
    //! Custom data for the Fft Pass.
    struct FftPassData
        : public AZ::RPI::PassData
    {
        AZ_RTTI(Ocean::FftPassData, "{EF1FBDD9-205D-46DC-A3BD-E0A29618931E}", AZ::RPI::PassData);
        AZ_CLASS_ALLOCATOR(FftPassData, AZ::SystemAllocator, 0);

        FftPassData() = default;
        virtual ~FftPassData() = default;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FftPassData, AZ::RPI::PassData>()
                    ->Version(1)
                    ->Field("Size", &FftPassData::m_size)
                    ;
            }
        }

        uint32_t m_size = 256;
    };


    // Compute shader pass that calculates a fast Fourier transform
    class FftParentPass final
        : public AZ::RPI::ParentPass
    {
        AZ_RPI_PASS(FftParentPass);
        
        public:
            AZ_RTTI(Ocean::FftParentPass, "{B5A3FCC4-8017-4285-A2BA-11A8ABB7E816}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(FftParentPass, AZ::SystemAllocator, 0);
            virtual ~FftParentPass() = default;
            
            //! Creates an SsaoParentPass
            static AZ::RPI::Ptr<FftParentPass> Create(const AZ::RPI::PassDescriptor& descriptor);
            
        private:

            FftParentPass(const AZ::RPI::PassDescriptor& descriptor);
            
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
            AZ::RHI::ShaderInputNameIndex m_precomputeImageIndex = "m_precomputeImage";

            AZ::Data::Instance<AZ::RPI::StreamingImage> m_gaussianNoiseImage;
            AZ::Data::Instance<AZ::RPI::AttachmentImage> m_fftPrecomputeImage;
            AZ::RPI::Ptr<AZ::RPI::ComputePass> m_fftPrecomputePass;

            uint32_t m_size{ 256 };
    };
}
