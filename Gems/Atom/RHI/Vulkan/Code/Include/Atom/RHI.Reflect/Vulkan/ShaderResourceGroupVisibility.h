/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI.Reflect/ShaderStages.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

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
    }
}
