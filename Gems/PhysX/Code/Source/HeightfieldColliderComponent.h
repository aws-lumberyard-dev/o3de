/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>

#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>

#include <PhysX/ColliderComponentBus.h>
#include <PhysX/ColliderShapeBus.h>

#include <Source/Shape.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace PhysX
{
    class StaticRigidBody;

    //! Component that provides a Heightfield Collider.
    //! The heightfield collider component supports having its data changed at runtime.  It also relies on the HeightfieldProvider to
    //! provide its data.  Due to these differences, this component directly implements both the collider and static rigid body services -
    //! the default behaviors provided by BaseColliderComponent and StaticRigidBodyComponent don't line up well with the additional
    //! functionality required here.
    class HeightfieldColliderComponent
        : public AZ::Component
        , public ColliderComponentRequestBus::Handler
        , public AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , protected PhysX::ColliderShapeRequestBus::Handler
        , protected Physics::CollisionFilteringRequestBus::Handler
        , protected Physics::HeightfieldProviderNotificationBus::Handler

    {
    public:
        using Configuration = Physics::HeightfieldShapeConfiguration;
        AZ_COMPONENT(HeightfieldColliderComponent, "{9A42672C-281A-4CE8-BFDD-EAA1E0FCED76}");
        static void Reflect(AZ::ReflectContext* context);

        HeightfieldColliderComponent() = default;
        ~HeightfieldColliderComponent() override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        void Activate() override;
        void Deactivate() override;

        void SetShapeConfiguration(const AzPhysics::ShapeColliderPair& shapeConfig);

        // ColliderComponentRequestBus
        AzPhysics::ShapeColliderPairList GetShapeConfigurations() override;
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> GetShapes() override;

        // PhysX::ColliderShapeBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        // CollisionFilteringRequestBus
        void SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 filterTag) override;
        AZStd::string GetCollisionLayerName() override;
        void SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 filterTag) override;
        AZStd::string GetCollisionGroupName() override;
        void ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 filterTag, bool enabled) override;

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;

        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

    private:
        void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion) override;
        void RefreshHeightfield();

        void ClearHeightfieldData();
        bool InitShapes();
        void InitStaticRigidBody();


        /// Updates the scale of shape configurations to reflect the scale from the transform component.
        /// Specific collider components should override this function.
        void UpdateScaleForShapeConfigs();

        AzPhysics::ShapeColliderPair m_shapeConfig;
        AZStd::shared_ptr<Physics::Shape> m_heightfield;
        AzPhysics::SimulatedBodyHandle m_staticRigidBodyHandle = AzPhysics::InvalidSimulatedBodyHandle;
        AzPhysics::SceneHandle m_attachedSceneHandle = AzPhysics::InvalidSceneHandle;
    };
} // namespace PhysX
