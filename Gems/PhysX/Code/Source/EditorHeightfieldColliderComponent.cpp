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
    {
        m_colliderConfig.SetPropertyVisibility(Physics::ColliderConfiguration::Offset, false);
    }

    EditorHeightfieldColliderComponent ::~EditorHeightfieldColliderComponent()
    {
        m_shapeConfig->SetCachedNativeHeightfield(nullptr);
    }


    void EditorHeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorHeightfieldColliderComponent, EditorComponentBase>()
                ->Version(1)
                ->Field("ColliderConfiguration", &EditorHeightfieldColliderComponent::m_colliderConfig)
                ->Field("DebugDrawSettings", &EditorHeightfieldColliderComponent::m_colliderDebugDraw)
                ->Field("ShapeConfig", &EditorHeightfieldColliderComponent::m_shapeConfig)
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
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void EditorHeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
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

    void EditorHeightfieldColliderComponent::CreateStaticEditorCollider()
    {
        // Don't create static rigid body in the editor if current entity components
        // don't allow creation of runtime static rigid body component
        if (!StaticRigidBodyUtils::CanCreateRuntimeComponent(*GetEntity()))
        {
            return;
        }

        AZ::Transform colliderTransform = GetWorldTM();

        Physics::HeightfieldProviderRequestsBus::BroadcastResult(
            colliderTransform, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldTransform);

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

    AZ::u32 EditorHeightfieldColliderComponent::OnConfigurationChanged()
    {
        m_colliderConfig.m_materialSelection.SetMaterialSlots(Physics::MaterialSelection::SlotsArray());
        CreateStaticEditorCollider();
        return AZ::Edit::PropertyRefreshLevels::None;
    }

    void EditorHeightfieldColliderComponent::UpdateConfig()
    {
        Physics::HeightfieldShapeConfiguration& configuration =
            static_cast<Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig);
        configuration.SetCachedNativeHeightfield(nullptr);
        configuration = Physics::HeightfieldShapeConfiguration(GetEntityId());

        AZ::Vector2 gridSpacing(1.0f);
        Physics::HeightfieldProviderRequestsBus::EventResult(
            gridSpacing, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSpacing);

        configuration.SetGridResolution(gridSpacing);

        int32_t numRows = 0;
        int32_t numColumns = 0;
        Physics::HeightfieldProviderRequestsBus::Event(
            GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSize, numColumns, numRows);

        configuration.SetNumRows(numRows);
        configuration.SetNumColumns(numColumns);

        AZStd::vector<Physics::HeightMaterialPoint> samples;
        Physics::HeightfieldProviderRequestsBus::EventResult(samples,
            GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

        configuration.SetSamples(samples);

        // Scale is expected to be handled inside the heightfield provider, so we'll just set this to unit scale.
        m_shapeConfig->m_scale = AZ::Vector3::CreateOne();

    }

    // AZ::Component
    void EditorHeightfieldColliderComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        PhysX::ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());

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

        PhysX::ColliderShapeRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect();

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

    void EditorHeightfieldColliderComponent::OnHeightfieldTransformChanged([[maybe_unused]] const AZ::Transform& transform)
    {
        UpdateConfig();

        CreateStaticEditorCollider();
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
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

    // DisplayCallback
    void EditorHeightfieldColliderComponent::Display(AzFramework::DebugDisplayRequests& debugDisplay) const
    {
        const auto& heightfieldConfig = static_cast<const Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig);
        m_colliderDebugDraw.DrawHeightfield(debugDisplay, m_colliderConfig, heightfieldConfig);
    }

    // ColliderShapeRequestBus
    AZ::Aabb EditorHeightfieldColliderComponent::GetColliderShapeAabb()
    {
        AZ::Aabb aabb = AZ::Aabb::CreateFromPoint(GetWorldTM().GetTranslation());

        Physics::HeightfieldProviderRequestsBus::EventResult(
            aabb, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldAabb);

        return aabb;
    }

    bool EditorHeightfieldColliderComponent::IsTrigger()
    {
        return m_colliderConfig.m_isTrigger;
    }

    void EditorHeightfieldColliderComponent::OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        RefreshHeightfield();
    }

    void EditorHeightfieldColliderComponent::RefreshHeightfield()
    {
        UpdateConfig();

        CreateStaticEditorCollider();
        Physics::ColliderComponentEventBus::Event(GetEntityId(), &Physics::ColliderComponentEvents::OnColliderChanged);
    }


} // namespace PhysX
