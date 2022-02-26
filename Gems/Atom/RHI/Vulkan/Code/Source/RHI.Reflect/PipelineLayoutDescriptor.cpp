/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/PipelineLayoutDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace Vulkan
    {
        void ShaderResourceGroupVisibility::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderResourceGroupVisibility>()
                    ->Version(0)
                    ->Field("m_resourcesStageMask", &ShaderResourceGroupVisibility::m_resourcesStageMask)
                    ->Field("m_constantDataStageMask", &ShaderResourceGroupVisibility::m_constantDataStageMask)
                    ;
            }
        }

        HashValue64 ShaderResourceGroupVisibility::GetHash(HashValue64 seed) const
        {
            HashValue64 hash = seed;
            hash = TypeHash64(m_constantDataStageMask, hash);
            for (const auto& it : m_resourcesStageMask)
            {
                hash = TypeHash64(it.first.GetCStr(), hash);
                hash = TypeHash64(it.second, hash);
            }

            return hash;
        }

        void PipelineLayoutDescriptor::Reflect(AZ::ReflectContext* context)
        {
            ShaderResourceGroupVisibility::Reflect(context);

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
