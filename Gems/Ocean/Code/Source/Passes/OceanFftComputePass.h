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
    struct OceanFftComputePassData
        : public AZ::RPI::ComputePassData
    {
        AZ_RTTI(Ocean::OceanFftComputePassData, "{68EBEE18-1771-4A74-93F9-245D852E9E6F}", AZ::RPI::ComputePassData);
        AZ_CLASS_ALLOCATOR(Ocean::OceanFftComputePassData, AZ::SystemAllocator, 0);

        OceanFftComputePassData() = default;
        virtual ~OceanFftComputePassData() = default;

        static void Reflect(AZ::ReflectContext* context);
    };

    class OceanFftComputePass
        : public AZ::RPI::ComputePass
    {
        AZ_RPI_PASS(OceanFftComputePass);

    public:
        AZ_RTTI(Ocean::OceanFftComputePass, "{027FF6DC-4D35-4633-82AB-714742FAE535}", AZ::RPI::ComputePass);
        AZ_CLASS_ALLOCATOR(Ocean::OceanFftComputePass, AZ::SystemAllocator, 0);

        virtual ~OceanFftComputePass() = default;

        static AZ::RPI::Ptr<OceanFftComputePass> Create(const AZ::RPI::PassDescriptor& descriptor);

        void CompileResources(const AZ::RHI::FrameGraphCompileContext& context) override;

    private:

        OceanFftComputePass(const AZ::RPI::PassDescriptor& descriptor);
        void BuildCommandListInternal(const AZ::RHI::FrameGraphExecuteContext& context) override;

    };
} // namespace AZ::Render
