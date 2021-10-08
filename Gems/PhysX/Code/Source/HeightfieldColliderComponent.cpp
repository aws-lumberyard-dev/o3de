/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <Source/HeightfieldColliderComponent.h>
#include <Source/Utils.h>

namespace PhysX
{

    HeightfieldColliderComponent::HeightfieldColliderComponent()
        : HeightfieldColliderComponentCommon(GetEntityId())
    {
    }

    void HeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HeightfieldColliderComponent, BaseColliderComponent>()
                ->Version(1)
                ;
        }
    }

    void HeightfieldColliderComponent::Activate()
    {
        BaseColliderComponent::Activate();
        HeightfieldColliderComponentCommon::Activate();
    }

    void HeightfieldColliderComponent::Deactivate()
    {
        HeightfieldColliderComponentCommon::Deactivate();
        BaseColliderComponent::Deactivate();
    }

    void HeightfieldColliderComponent::UpdateScaleForShapeConfigs()
    {
        if (m_shapeConfigList.size() != 1)
        {
            AZ_Error(
                "PhysX Heightfield Collider Component", false, "Expected exactly one collider/shape configuration for entity \"%s\".",
                GetEntity()->GetName().c_str());
            return;
        }

        m_shapeConfigList[0].second->m_scale = Utils::GetTransformScale(GetEntityId());
    }
} // namespace PhysX
