/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <Editor/DebugDraw.h>
#include <PhysX/ColliderShapeBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace PhysX
{
    //! Editor PhysX Heightfield Collider Component.
    class EditorHeightfieldColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , protected Physics::HeightfieldProviderNotificationBus::Handler
        , protected AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private PhysX::ColliderShapeRequestBus::Handler
        , protected LmbrCentral::ShapeComponentNotificationsBus::Handler
        , protected DebugDraw::DisplayCallback
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorHeightfieldColliderComponent,
            "{C388C3DB-8D2E-4D26-96D3-198EDC799B77}",
            AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        EditorHeightfieldColliderComponent();

        // Helper for tests
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> GetShapeConfig();

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
    private:
        AZ::u32 OnConfigurationChanged();
        void UpdateConfig();
        void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion) override;
        void RefreshHeightfield() override;
        void CreateStaticEditorCollider();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // handling for non-uniform scale
        void OnNonUniformScaleChanged(const AZ::Vector3& scale);

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // ColliderShapeRequestBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        // DisplayCallback
        void Display(AzFramework::DebugDisplayRequests& debugDisplay) const;

        // LmbrCentral::ShapeComponentNotificationBus
        void OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons changeReason) override;

        Physics::ColliderConfiguration m_colliderConfig; //!< Stores collision layers, whether the collider is a trigger, etc.
        DebugDraw::Collider m_colliderDebugDraw; //!< Handles drawing the collider based on global and local
        AzPhysics::SceneInterface* m_sceneInterface{ nullptr };
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> m_shapeConfig{ new Physics::HeightfieldShapeConfiguration() };
        AzPhysics::SimulatedBodyHandle m_editorBodyHandle =
            AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to the body in the editor physics scene if there is no rigid body component.
        AzPhysics::SceneHandle m_editorSceneHandle = AzPhysics::InvalidSceneHandle;
        AZ::NonUniformScaleChangedEvent::Handler m_nonUniformScaleChangedHandler; //!< Responds to changes in non-uniform scale.
        AZ::Transform m_cachedWorldTransform;
        AZ::Vector3 m_currentNonUniformScale = AZ::Vector3::CreateOne(); //!< Caches the current non-uniform scale.

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
        AzPhysics::SystemEvents::OnMaterialLibraryChangedEvent::Handler m_onMaterialLibraryChangedEventHandler;
    };

} // namespace PhysX
