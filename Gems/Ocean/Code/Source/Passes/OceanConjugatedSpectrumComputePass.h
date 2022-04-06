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
    struct OceanConjugatedSpectrumComputePassData
        : public AZ::RPI::ComputePassData
    {
        AZ_RTTI(Ocean::OceanConjugatedSpectrumComputePassData, "{8AB37243-4C5D-4ABB-A55E-75D2335D5D9A}", AZ::RPI::ComputePassData);
        AZ_CLASS_ALLOCATOR(Ocean::OceanConjugatedSpectrumComputePassData, AZ::SystemAllocator, 0);

        OceanConjugatedSpectrumComputePassData() = default;
        virtual ~OceanConjugatedSpectrumComputePassData() = default;

        static void Reflect(AZ::ReflectContext* context);
    };

    class OceanConjugatedSpectrumComputePass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(OceanFftComputePass);

    public:
        AZ_RTTI(Ocean::OceanConjugatedSpectrumComputePass, "{2C4C2016-5DFF-45C1-83AB-30553C8EBBEC}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Ocean::OceanConjugatedSpectrumComputePass, AZ::SystemAllocator, 0);

        virtual ~OceanConjugatedSpectrumComputePass() = default;

        static AZ::RPI::Ptr<OceanConjugatedSpectrumComputePass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:

        OceanConjugatedSpectrumComputePass(const AZ::RPI::PassDescriptor& descriptor);
        void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

        void UpdateSrg();

        AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_constants";
    };
} // namespace AZ::Render
