/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;
    
    namespace Vulkan
    {        
        //! This class describes the usage mask for an Shader Resource
        //! Group that is part of a Pipeline.
        //! Contains a mask that describes in which shader stage a resource
        //! is being used.
        class ShaderResourceGroupVisibility
            : public AZStd::intrusive_base
        {
        public:
            AZ_TYPE_INFO(ShaderResourceGroupVisibility, "{64279248-DD42-46CA-BEA7-4058E59B4D35}");
            AZ_CLASS_ALLOCATOR(ShaderResourceGroupVisibility, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            ShaderResourceGroupVisibility() = default;
            HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

            //! Shader usage mask for each resource.
            AZStd::unordered_map<AZ::Name, RHI::ShaderStageMask> m_resourcesStageMask;

            //! Shader usage mask for the constant data. All constants share the same usage mask.
            RHI::ShaderStageMask m_constantDataStageMask = RHI::ShaderStageMask::None;
        };
    
        class PipelineLayoutDescriptor final
            : public RHI::PipelineLayoutDescriptor
        {
            using Base = RHI::PipelineLayoutDescriptor;
        public:
            AZ_RTTI(PipelineLayoutDescriptor, "{27E91BEA-A630-44DA-B430-8B6E0507EE45}", Base);
            AZ_CLASS_ALLOCATOR(PipelineLayoutDescriptor, AZ::SystemAllocator, 0);
            static void Reflect(AZ::ReflectContext* context);

            static RHI::Ptr<PipelineLayoutDescriptor> Create();

            void AddShaderResourceGroupVisibility(RHI::Ptr<ShaderResourceGroupVisibility> visibilityInfo);
            const ShaderResourceGroupVisibility* GetShaderResourceGroupVisibility(uint32_t index) const;

        private:

            PipelineLayoutDescriptor() = default;
            
            //////////////////////////////////////////////////////////////////////////
            // RHI::PipelineLayoutDescriptor overrides
            void ResetInternal() override;
            HashValue64 GetHashInternal(HashValue64 seed) const override;
            //////////////////////////////////////////////////////////////////////////

            AZ_SERIALIZE_FRIEND();

            AZStd::fixed_vector<RHI::Ptr<ShaderResourceGroupVisibility>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroupVisibilities;
        };
    }
}
