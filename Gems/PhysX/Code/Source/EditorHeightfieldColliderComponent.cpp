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

#include <System/PhysXSystem.h>

namespace PhysX
{
    EditorHeightfieldColliderComponent::EditorHeightfieldColliderComponent()
        : m_physXConfigChangedHandler(
              []([[maybe_unused]] const AzPhysics::SystemConfiguration* config)
              {
                  AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                      &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                      AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
              })
        , m_onMaterialLibraryChangedEventHandler(
              [this](const AZ::Data::AssetId& defaultMaterialLibrary)
              {
                  m_colliderConfig.m_materialSelection.OnMaterialLibraryChanged(defaultMaterialLibrary);
                  Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);

                  AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                      &AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                      AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
              })
        , m_nonUniformScaleChangedHandler(
              [this](const AZ::Vector3& scale)
              {
                  OnNonUniformScaleChanged(scale);
              })
    {
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
    }

    void EditorHeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorHeightfieldColliderComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("ColliderConfiguration", &EditorHeightfieldColliderComponent::m_colliderConfig)
                ->Field("DebugDrawSettings", &EditorHeightfieldColliderComponent::m_colliderDebugDraw)
                ->Field("ShapeConfigs", &EditorHeightfieldColliderComponent::m_shapeConfig)
                ;

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
                            AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/physx/heightfield-collider/")
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

    void EditorHeightfieldColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysXColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXTriggerService"));
        provided.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
    }

    void EditorHeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("AxisAlignedBoxShapeService"));
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void EditorHeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
    }

    // AZ::Component
    void EditorHeightfieldColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        PhysX::ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());
        AZ::NonUniformScaleRequestBus::Event(
            GetEntityId(), &AZ::NonUniformScaleRequests::RegisterScaleChangedEvent, m_nonUniformScaleChangedHandler);

        AZ::TransformBus::EventResult(m_cachedWorldTransform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        m_currentNonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(m_currentNonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::MaterialSelection, false);

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
        AzToolsFramework::Components::EditorComponentBase::Deactivate();

        if (m_sceneInterface && m_editorBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            m_sceneInterface->RemoveSimulatedBody(m_editorSceneHandle, m_editorBodyHandle);
        }
    }

    // AzToolsFramework::EntitySelectionEvents
    void EditorHeightfieldColliderComponent::OnSelected()
    {
        if (auto* physXSystem = GetPhysXSystem())
        {
            if (!m_physXConfigChangedHandler.IsConnected())
            {
                physXSystem->RegisterSystemConfigurationChangedEvent(m_physXConfigChangedHandler);
            }
            if (!m_onMaterialLibraryChangedEventHandler.IsConnected())
            {
                physXSystem->RegisterOnMaterialLibraryChangedEventHandler(m_onMaterialLibraryChangedEventHandler);
            }
        }
    }

    void EditorHeightfieldColliderComponent::OnDeselected()
    {
        m_onMaterialLibraryChangedEventHandler.Disconnect();
        m_physXConfigChangedHandler.Disconnect();
    }

    void EditorHeightfieldColliderComponent::OnNonUniformScaleChanged(const AZ::Vector3& scale)
    {
        m_currentNonUniformScale = scale;

        UpdateConfig();

        CreateStaticEditorCollider();
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }

    void EditorHeightfieldColliderComponent::OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        RefreshHeightfield();
    }

    void EditorHeightfieldColliderComponent::OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        if (changeReason == LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            UpdateConfig();

            CreateStaticEditorCollider();
            Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
        }
    }

    void EditorHeightfieldColliderComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto* shapeColliderComponent = gameEntity->CreateComponent<HeightfieldColliderComponent>();
        AzPhysics::ShapeColliderPairList shapeConfigurationList;
        shapeConfigurationList.reserve(1);
        shapeConfigurationList.emplace_back(AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig), m_shapeConfig);

        shapeColliderComponent->SetShapeConfigurationList(shapeConfigurationList);

        StaticRigidBodyUtils::TryCreateRuntimeComponent(*GetEntity(), *gameEntity);
    }

    void EditorHeightfieldColliderComponent::UpdateConfig()
    {
        const AZ::Vector3 uniformScale = Utils::GetUniformScale(GetEntityId());
        const AZ::Vector3 overallScale = uniformScale * m_currentNonUniformScale;

        Physics::HeightfieldShapeConfiguration& configuration =
            static_cast<Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig);
        configuration = Physics::HeightfieldShapeConfiguration(GetEntityId());

        m_shapeConfig->m_scale = overallScale;
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
        colliderShapePairs.reserve(1);
        colliderShapePairs.emplace_back(AZStd::make_shared<Physics::ColliderConfiguration>(m_colliderConfig), m_shapeConfig);
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

    // ColliderShapeRequestBus
    AZ::Aabb EditorHeightfieldColliderComponent::GetColliderShapeAabb()
    {
        AZ::Aabb aabb = AZ::Aabb::CreateFromPoint(GetWorldTM().GetTranslation());
        LmbrCentral::ShapeComponentRequestsBus::EventResult(aabb, GetEntityId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);
        return aabb;
    }

    bool EditorHeightfieldColliderComponent::IsTrigger()
    {
        return m_colliderConfig.m_isTrigger;
    }

    // DisplayCallback
    void EditorHeightfieldColliderComponent::Display(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const auto& heightfieldConfig = static_cast<const Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig);
        m_colliderDebugDraw.DrawHeightfield(debugDisplay, m_colliderConfig, heightfieldConfig);
    }

} // namespace PhysX
