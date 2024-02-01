/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LevelBuilderComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/API/ApplicationAPI.h>

namespace LevelBuilder
{
    // TODO - Remove this?
    void LevelBuilderComponent::Activate()
    {
        return;
    }

    void LevelBuilderComponent::Deactivate()
    {
        m_levelBuilder.BusDisconnect();
    }

    void LevelBuilderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LevelBuilderComponent, AZ::Component>()
                ->Version(2)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }
}
