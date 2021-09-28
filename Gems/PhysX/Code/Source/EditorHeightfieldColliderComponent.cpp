/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <EditorHeightfieldColliderComponent.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <Source/HeightfieldColliderComponent.h>
#include <Source/Utils.h>
#include <AzFramework/Physics/Shape.h>
#include <Editor/ColliderComponentMode.h>

namespace PhysX
{
    void EditorHeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorHeightfieldColliderComponent, EditorComponentBase>()->Version(1);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorHeightfieldColliderComponent>(
                    "PhysX Heightfield Collider", "Creates geometry in the PhysX simulation based on an attached heightfield component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/PhysXCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/PhysXCollider.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(
                            AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/shape-collider/")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorHeightfieldColliderComponent::m_colliderConfig, "Collider configuration",
                        "Configuration of the collider")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorHeightfieldColliderComponent::OnConfigurationChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorHeightfieldColliderComponent::m_colliderDebugDraw, "Debug draw settings",
                        "Debug draw settings")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void EditorHeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        required.push_back(AZ_CRC("AxisAlignedBoxShapeService", 0xe86aa5fe));
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    // AZ::Component
    void EditorHeightfieldColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        PhysX::ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());
        AZ::NonUniformScaleRequestBus::Event(
            GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);

        AZ::TransformBus::EventResult(m_cachedWorldTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_currentNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_currentNonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        m_sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        if (m_sceneInterface)
        {
            m_editorSceneHandle = m_sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
        }

        UpdateConfig();

        // Debug drawing
        m_colliderDebugDraw.Connect(GetEntityId());
        m_colliderDebugDraw.SetDisplayCallback(this);
        CreateStaticEditorCollider();

        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void EditorHeightfieldColliderComponent::Deactivate()
    {
        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();
        m_colliderDebugDraw.Disconnect();

        m_nonUniformScaleChangedHandler.Disconnect();
        PhysX::ColliderShapeRequestBus::Handler::BusDisconnect();
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    void EditorHeightfieldColliderComponent::OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        RefreshHeightfield();
    }

    void EditorHeightfieldColliderComponent::UpdateConfig()
    {
        m_geometryCache.m_cachedSamplePointsDirty = true;

        const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
        const AZ::Vector3 overallScale = uniformScale * m_currentNonUniformScale;

        overallScale;

    }

    void EditorHeightfieldColliderComponent::RefreshHeightfield()
    {
        CreateStaticEditorCollider();
    }

    void EditorHeightfieldColliderComponent::CreateStaticEditorCollider()
    {
        // Don't create static rigid body in the editor if current entity components
        // don't allow creation of runtime static rigid body component
        if (!StaticRigidBodyUtils::CanCreateRuntimeComponent(*GetEntity()))
        {
            return;
        }

        const AZ::Transform colliderTransform = GetWorldTM();

        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = colliderTransform.GetRotation();
        configuration.m_position = colliderTransform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();

        AzPhysics::ShapeColliderPairList colliderShapePairs;
        colliderShapePairs.reserve(m_shapeConfigs.size());
        for (const auto& shapeConfig : m_shapeConfigs)
        {
            colliderShapePairs.emplace_back(AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig), shapeConfig);
        }
        configuration.m_colliderAndShapeData = colliderShapePairs;

        if (m_sceneInterface)
        {
            // remove the previous body if any
            if (m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
            {
                m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
            }

            m_editorBodyHandle = m_sceneInterface->AddSimulatedBody(m_editorSceneHandle, &configuration);
        }

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

        void EditorHeightfieldColliderComponent::EnablePhysics()
    {
        if (!IsPhysicsEnabled() && m_sceneInterface)
        {
            m_sceneInterface->EnableSimulationOfBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    void EditorHeightfieldColliderComponent::DisablePhysics()
    {
        if (m_sceneInterface)
        {
            m_sceneInterface->DisableSimulationOfBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    bool EditorHeightfieldColliderComponent::IsPhysicsEnabled() const
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->m_simulating;
            }
        }
        return false;
    }

        AZ::Aabb EditorHeightfieldColliderComponent::GetAabb() const
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SimulatedBody* EditorHeightfieldColliderComponent::GetSimulatedBody()
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body;
            }
        }
        return nullptr;
    }

        AzPhysics::SimulatedBodyHandle EditorHeightfieldColliderComponent::GetSimulatedBodyHandle() const
    {
        return m_editorBodyHandle;
    }

    AzPhysics::SceneQueryHit EditorHeightfieldColliderComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* body = m_sceneInterface->GetSimulatedBodyFromHandle(m_editorSceneHandle, m_editorBodyHandle))
            {
                return body->RayCast(request);
            }
        }
        return AzPhysics::SceneQueryHit();
    }

    AZ::u32 EditorHeightfieldColliderComponent::OnConfigurationChanged()
    {
        m_colliderConfig.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        CreateStaticEditorCollider();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

} // namespace PhysX
