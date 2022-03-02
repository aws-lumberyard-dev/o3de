/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderResourceGroupPoolDescriptor.h>

namespace AZ
{
    namespace Vulkan
    {
        class ShaderResourceGroupVisibility;

        class ShaderResourceGroupPoolDescriptor final
            : public RHI::ShaderResourceGroupPoolDescriptor
        {
            using Base = RHI::ShaderResourceGroupPoolDescriptor;
        public:
            AZ_RTTI(ShaderResourceGroupPoolDescriptor, "{8E219121-81FE-42FA-B68A-241F40ECB01D}", Base);

            ShaderResourceGroupPoolDescriptor() = default;
            
            const ShaderResourceGroupVisibility* m_shaderResouceGroupVisibility = nullptr;
        };

    }
}
