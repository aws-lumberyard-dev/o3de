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

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Utils.h>

#include <Source/BaseColliderComponent.h>
#include <Source/RigidBodyStatic.h>
#include <Source/StaticRigidBodyComponent.h>
#include <Source/SystemComponent.h>
#include <Source/Utils.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <Scene/PhysXScene.h>

#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Utils.h>

#include <PhysX/ColliderComponentBus.h>


namespace PhysX
{
    void HeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HeightfieldColliderComponent, AZ::Component>()
                ->Version(1)
                ->Field("ShapeConfig", &HeightfieldColliderComponent::m_shapeConfig)
                ;
        }
    }

    void HeightfieldColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysXColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXTriggerService"));
        provided.push_back(AZ_CRC_CE("PhysXStaticRigidBodyService"));
    }

    void HeightfieldColliderComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsHeightfieldProviderService"));
    }

    void HeightfieldColliderComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXColliderService"));
        incompatible.push_back(AZ_CRC_CE("PhysXStaticRigidBodyService"));
        incompatible.push_back(AZ_CRC_CE("PhysXRigidBodyService"));
    }

    HeightfieldColliderComponent::~HeightfieldColliderComponent()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }

        ClearHeightfieldData();
    }

    void HeightfieldColliderComponent::InitStaticRigidBody()
    {
        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);

        Physics::HeightfieldProviderRequestsBus::BroadcastResult(
            transform, &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldTransform);

        AzPhysics::StaticRigidBodyConfiguration configuration;
        configuration.m_orientation = transform.GetRotation();
        configuration.m_position = transform.GetTranslation();
        configuration.m_entityId = GetEntityId();
        configuration.m_debugName = GetEntity()->GetName();
        configuration.m_colliderAndShapeData = GetShapeConfigurations();

        if (m_attachedSceneHandle == AzPhysics::InvalidSceneHandle)
        {
            Physics::DefaultWorldBus::BroadcastResult(m_attachedSceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        }
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_staticRigidBodyHandle = sceneInterface->AddSimulatedBody(m_attachedSceneHandle, &configuration);
        }
    }



    void HeightfieldColliderComponent::SetShapeConfiguration(const AzPhysics::ShapeColliderPair& shapeConfig)
    {
        if (GetEntity()->GetState() == AZ::Entity::State::Active)
        {
            AZ_Warning(
                "PhysX", false, "Trying to call SetShapeConfiguration for entity \"%s\" while entity is active.",
                GetEntity()->GetName().c_str());
            return;
        }
        m_shapeConfig = shapeConfig;
    }

    AzPhysics::ShapeColliderPairList HeightfieldColliderComponent::GetShapeConfigurations()
    {
        AzPhysics::ShapeColliderPairList shapeConfigurationList({m_shapeConfig});
        return shapeConfigurationList;
    }

    AZStd::vector<AZStd::shared_ptr<Physics::Shape>> HeightfieldColliderComponent::GetShapes()
    {
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapes({m_heightfield});
        return shapes;
    }

    // PhysX::ColliderShapeBus
    AZ::Aabb HeightfieldColliderComponent::GetColliderShapeAabb()
    {
        AZ::Aabb colliderAabb = AZ::Aabb::CreateNull();
        Physics::HeightfieldProviderRequestsBus::EventResult(
            colliderAabb, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldAabb);

        return colliderAabb;
    }

    bool HeightfieldColliderComponent::IsTrigger()
    {
        return (m_shapeConfig.first->m_isTrigger);
    }

    void HeightfieldColliderComponent::SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag)
    {
        if (Physics::Utils::FilterTag(m_heightfield->GetTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionLayer layer;
            Physics::CollisionRequestBus::BroadcastResult(
                success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, layer);
            if (success)
            {
                m_heightfield->SetCollisionLayer(layer);
            }
        }
    }

    AZStd::string HeightfieldColliderComponent::GetCollisionLayerName()
    {
        AZStd::string layerName;
        Physics::CollisionRequestBus::BroadcastResult(
            layerName, &Physics::CollisionRequests::GetCollisionLayerName, m_heightfield->GetCollisionLayer());
        return layerName;
    }

    void HeightfieldColliderComponent::SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag)
    {
        if (Physics::Utils::FilterTag(m_heightfield->GetTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionGroup group;
            Physics::CollisionRequestBus::BroadcastResult(
                success, &Physics::CollisionRequests::TryGetCollisionGroupByName, groupName, group);
            if (success)
            {
                m_heightfield->SetCollisionGroup(group);
            }
        }
    }

    AZStd::string HeightfieldColliderComponent::GetCollisionGroupName()
    {
        AZStd::string groupName;
        Physics::CollisionRequestBus::BroadcastResult(
            groupName, &Physics::CollisionRequests::GetCollisionGroupName, m_heightfield->GetCollisionGroup());

        return groupName;
    }

    void HeightfieldColliderComponent::ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled)
    {
        if (Physics::Utils::FilterTag(m_heightfield->GetTag(), colliderTag))
        {
            bool success = false;
            AzPhysics::CollisionLayer layer;
            Physics::CollisionRequestBus::BroadcastResult(
                success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, layer);
            if (success)
            {
                auto group = m_heightfield->GetCollisionGroup();
                group.SetLayer(layer, enabled);
                m_heightfield->SetCollisionGroup(group);
            }
        }
    }






    void HeightfieldColliderComponent::Activate()
    {
        Physics::HeightfieldProviderNotificationBus::Handler::BusConnect(GetEntityId());
        RefreshHeightfield();

        const AZ::EntityId entityId = GetEntityId();

        ColliderComponentRequestBus::Handler::BusConnect(entityId);
        ColliderShapeRequestBus::Handler::BusConnect(GetEntityId());
        Physics::CollisionFilteringRequestBus::Handler::BusConnect(GetEntityId());

        InitShapes();

        InitStaticRigidBody();

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusConnect(GetEntityId());

    }

    void HeightfieldColliderComponent::Deactivate()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }

        AzPhysics::SimulatedBodyComponentRequestsBus::Handler::BusDisconnect();

        ClearHeightfieldData();
        m_heightfield.reset();

        Physics::CollisionFilteringRequestBus::Handler::BusDisconnect();
        ColliderShapeRequestBus::Handler::BusDisconnect();
        ColliderComponentRequestBus::Handler::BusDisconnect();

        Physics::HeightfieldProviderNotificationBus::Handler::BusDisconnect();
    }

    void HeightfieldColliderComponent::ClearHeightfieldData()
    {
        Physics::HeightfieldShapeConfiguration& configuration =
            static_cast<Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig.second);

        configuration.SetCachedNativeHeightfield(nullptr);
    }

    void HeightfieldColliderComponent::UpdateScaleForShapeConfigs()
    {
        m_shapeConfig.second->m_scale = Utils::GetTransformScale(GetEntityId());
    }

    void HeightfieldColliderComponent::OnHeightfieldDataChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
    {
        RefreshHeightfield();
    }

    void HeightfieldColliderComponent::RefreshHeightfield()
    {
        Physics::HeightfieldShapeConfiguration& configuration =
            static_cast<Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig.second);

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
        Physics::HeightfieldProviderRequestsBus::EventResult(
            samples, GetEntityId(), &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials);

        configuration.SetSamples(samples);

        m_heightfield.reset();
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->RemoveSimulatedBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }

        InitShapes();
        InitStaticRigidBody();

    }

    void HeightfieldColliderComponent::EnablePhysics()
    {
        if (IsPhysicsEnabled())
        {
            return;
        }
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->EnableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    void HeightfieldColliderComponent::DisablePhysics()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            sceneInterface->DisableSimulationOfBody(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
    }

    bool HeightfieldColliderComponent::IsPhysicsEnabled() const
    {
        if (m_staticRigidBodyHandle != AzPhysics::InvalidSimulatedBodyHandle)
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
                sceneInterface != nullptr && sceneInterface->IsEnabled(m_attachedSceneHandle)) // check if the scene is enabled
            {
                if (AzPhysics::SimulatedBody* body =
                        sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
                {
                    return body->m_simulating;
                }
            }
        }
        return false;
    }

    AZ::Aabb HeightfieldColliderComponent::GetAabb() const
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            if (AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle))
            {
                return body->GetAabb();
            }
        }
        return AZ::Aabb::CreateNull();
    }

    AzPhysics::SimulatedBodyHandle HeightfieldColliderComponent::GetSimulatedBodyHandle() const
    {
        return m_staticRigidBodyHandle;
    }

    AzPhysics::SimulatedBody* HeightfieldColliderComponent::GetSimulatedBody()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            return sceneInterface->GetSimulatedBodyFromHandle(m_attachedSceneHandle, m_staticRigidBodyHandle);
        }
        return nullptr;
    }

    AzPhysics::SceneQueryHit HeightfieldColliderComponent::RayCast(const AzPhysics::RayCastRequest& request)
    {
        if (auto* body = azdynamic_cast<PhysX::StaticRigidBody*>(GetSimulatedBody()))
        {
            return body->RayCast(request);
        }
        return AzPhysics::SceneQueryHit();
    }

    bool HeightfieldColliderComponent::InitShapes()
    {
        UpdateScaleForShapeConfigs();

        const AZ::Vector3 nonUniformScale = Utils::GetTransformScale(GetEntityId());

        const AZStd::shared_ptr<Physics::ShapeConfiguration>& shapeConfiguration = m_shapeConfig.second;
        if (!shapeConfiguration)
        {
            AZ_Error(
                "PhysX", false, "Unable to create a physics shape because shape configuration is null. Entity: %s",
                GetEntity()->GetName().c_str());
            return false;
        }

        Physics::ColliderConfiguration colliderConfiguration = *m_shapeConfig.first;
        colliderConfiguration.m_position *= nonUniformScale;

        Physics::SystemRequestBus::BroadcastResult(
            m_heightfield, &Physics::SystemRequests::CreateShape, colliderConfiguration, *shapeConfiguration);

        if (!m_heightfield)
        {
            AZ_Error("PhysX", false, "Failed to create a PhysX shape. Entity: %s", GetEntity()->GetName().c_str());
            return false;
        }

        return true;
    }


} // namespace PhysX
