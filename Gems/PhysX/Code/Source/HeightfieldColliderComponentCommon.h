/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzFramework\Physics\HeightfieldProviderBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace PhysX
{
    //! Editor PhysX Heightfield Collider Component Common functions.
    class HeightfieldColliderComponentCommon
        : protected Physics::HeightfieldProviderNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(PhysX::HeightfieldColliderComponentCommon, "{BCBDF8BF-35B9-42BC-B283-66E7FB8E4963}");

        HeightfieldColliderComponentCommon() = default;
        HeightfieldColliderComponentCommon(AZ::EntityId entityId);


        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        // AZ::Component
        void Activate();
        void Deactivate();

    protected:
        void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion) override;
        void RefreshHeightfield() override;

        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> m_shapeConfig{ new Physics::HeightfieldShapeConfiguration() };

    private:
        AZ::EntityId m_entityId;

    };

} // namespace PhysX
