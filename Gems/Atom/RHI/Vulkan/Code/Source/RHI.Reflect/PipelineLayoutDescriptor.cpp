/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/ShaderResourceGroupVisibility.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace Vulkan
    {
        void PipelineLayoutDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PipelineLayoutDescriptor, Base>()
                    ->Version(1)
                    ->Field("m_shaderResourceGroupVisibilities", &PipelineLayoutDescriptor::m_shaderResourceGroupVisibilities);
            }
        }
        
        void PipelineLayoutDescriptor::AddShaderResourceGroupVisibility(RHI::Ptr<ShaderResourceGroupVisibility> visibilityInfo)
        {
            m_shaderResourceGroupVisibilities.push_back(visibilityInfo);
        }

        const ShaderResourceGroupVisibility* PipelineLayoutDescriptor::GetShaderResourceGroupVisibility(uint32_t index) const
        {
            return m_shaderResourceGroupVisibilities[index].get();
        }
        
        RHI::Ptr<PipelineLayoutDescriptor> PipelineLayoutDescriptor::Create()
        {
            return aznew PipelineLayoutDescriptor();
        }

        void PipelineLayoutDescriptor::ResetInternal()
        {
            m_shaderResourceGroupVisibilities.clear();
        }
        
        HashValue64 PipelineLayoutDescriptor::GetHashInternal(HashValue64 seed) const
        {
            HashValue64 hash = seed;
            
            for (const auto& visibilityInfo : m_shaderResourceGroupVisibilities)
            {
                hash = TypeHash64(visibilityInfo->GetHash(), hash);
            }

            return hash;
        }
    }
}
