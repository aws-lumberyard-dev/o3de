/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <HeightfieldColliderComponentCommon.h>

namespace PhysX
{
    HeightfieldColliderComponentCommon::HeightfieldColliderComponentCommon(AZ::EntityId entityId)
        : m_entityId{entityId}
    {
    }

    void HeightfieldColliderComponentCommon::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HeightfieldColliderComponentCommon>()
                ->Version(1)
                ->Field("ShapeConfigs", &HeightfieldColliderComponentCommon::m_shapeConfig)
                ;
        }
    }


    void HeightfieldColliderComponentCommon::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysXColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXTriggerService"));
        provided.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
    }

    void HeightfieldColliderComponentCommon::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void HeightfieldColliderComponentCommon::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
    }

    void HeightfieldColliderComponentCommon::Activate()
    {
        Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(m_entityId);
    }

    void HeightfieldColliderComponentCommon::Deactivate()
    {
        Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect(m_entityId);
    }

    void HeightfieldColliderComponentCommon::RefreshHeightfield()
    {
    }

} // namespace PhysX
