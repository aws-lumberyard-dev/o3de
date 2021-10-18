/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <Editor/DebugDraw.h>
#include <PhysX/ColliderShapeBus.h>
#include <AzFramework/Physics/HeightfieldProviderBus.h>

namespace PhysX
{
    //! Editor PhysX Heightfield Collider Component.
    class EditorHeightfieldColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , protected AzPhysics::SimulatedBodyComponentRequestsBus::Handler
        , private PhysX::ColliderShapeRequestBus::Handler
        , protected DebugDraw::DisplayCallback
        , protected Physics::HeightfieldProviderNotificationBus::Handler
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
        ~EditorHeightfieldColliderComponent();

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
    private:
        void CreateStaticEditorCollider();
        AZ::u32 OnConfigurationChanged();
        void UpdateConfig();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // AzPhysics::SimulatedBodyComponentRequestsBus::Handler overrides ...
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        AzPhysics::SimulatedBody* GetSimulatedBody() override;
        AzPhysics::SimulatedBodyHandle GetSimulatedBodyHandle() const override;
        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // DisplayCallback
        void Display(AzFramework::DebugDisplayRequests& debugDisplay) const;

        // ColliderShapeRequestBus
        AZ::Aabb GetColliderShapeAabb() override;
        bool IsTrigger() override;

        // Physics::HeightfieldProviderNotificationBus
        void OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion) override;
        void RefreshHeightfield();

        void ClearHeightfieldData();

        Physics::ColliderConfiguration m_colliderConfig; //!< Stores collision layers, whether the collider is a trigger, etc.
        DebugDraw::Collider m_colliderDebugDraw; //!< Handles drawing the collider based on global and local
        AzPhysics::SceneInterface* m_sceneInterface{ nullptr };
        AzPhysics::SceneHandle m_editorSceneHandle = AzPhysics::InvalidSceneHandle;
        AzPhysics::SimulatedBodyHandle m_editorBodyHandle =
            AzPhysics::InvalidSimulatedBodyHandle; //!< Handle to the body in the editor physics scene if there is no rigid body component.
        AZStd::shared_ptr<Physics::HeightfieldShapeConfiguration> m_shapeConfig{ new Physics::HeightfieldShapeConfiguration() };

        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;
        AzPhysics::SystemEvents::OnMaterialLibraryChangedEvent::Handler m_onMaterialLibraryChangedEventHandler;
    };

} // namespace PhysX
