/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/ShaderResourceGroupVisibility.h>
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
    }
}
