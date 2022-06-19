/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/HeightfieldColliderComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/Utils.h>

#include <Source/RigidBodyStatic.h>
#include <Source/SystemComponent.h>
#include <Source/Utils.h>

#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <Scene/PhysXScene.h>

namespace PhysX
{
    void HeightfieldColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<HeightfieldColliderComponent, AZ::Component>()
                ->Version(2)
                ->Field("ColliderConfiguration", &HeightfieldColliderComponent::m_colliderConfig)
                ->Field("BakedHeightfieldAsset", &HeightfieldColliderComponent::m_bakedHeightfieldAsset)
                ;
        }
    }

    void HeightfieldColliderComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsWorldBodyService"));
        provided.push_back(AZ_CRC_CE("PhysXColliderService"));
        provided.push_back(AZ_CRC_CE("PhysXHeightfieldColliderService"));
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
    }


    void HeightfieldColliderComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_bakedHeightfieldAsset)
        {
            m_bakedHeightfieldAsset = asset;

            Physics::HeightfieldShapeConfiguration& configuration = static_cast<Physics::HeightfieldShapeConfiguration&>(*m_shapeConfig.second);
            configuration = Utils::CreateBaseHeightfieldShapeConfiguration(GetEntityId());

            // Update material selection from the mapping
            Physics::ColliderConfiguration* colliderConfig = m_shapeConfig.first.get();
            Utils::SetMaterialsFromHeightfieldProvider(GetEntityId(), colliderConfig->m_materialSelection);

            Pipeline::HeightFieldAsset* heightfieldAsset = m_bakedHeightfieldAsset.Get();

            float minHeight = heightfieldAsset->GetMinHeight();
            float maxHeight = heightfieldAsset->GetMaxHeight();

            bool minMaxHeightsMatch = AZ::IsClose(configuration.GetMinHeightBounds(), minHeight)
                && AZ::IsClose(configuration.GetMaxHeightBounds(), maxHeight);

            if (minMaxHeightsMatch)
            {
                physx::PxHeightField* pxHeightfield = heightfieldAsset->GetHeightField();
                pxHeightfield->acquireReference();
                configuration.SetCachedNativeHeightfield(pxHeightfield);
                InitStaticRigidBody();
            }
        }
    }

    void HeightfieldColliderComponent::Activate()
    {
        const AZ::EntityId entityId = GetEntityId();

        AzPhysics::SceneHandle sceneHandle = AzPhysics::InvalidSceneHandle;
        Physics::DefaultWorldBus::BroadcastResult(sceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);

        m_heightfieldCollider =
            AZStd::make_unique<HeightfieldCollider>(GetEntityId(), GetEntity()->GetName(), sceneHandle, m_colliderConfig, m_shapeConfig);

        ColliderComponentRequestBus::Handler::BusConnect(entityId);
        Physics::CollisionFilteringRequestBus::Handler::BusConnect(entityId);

        if (m_bakedHeightfieldAsset.GetId().IsValid())
        {
            if (m_bakedHeightfieldAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::Error ||
                m_bakedHeightfieldAsset.GetStatus() == AZ::Data::AssetData::AssetStatus::NotLoaded)
            {
                m_bakedHeightfieldAsset.QueueLoad();
            }

            AZ::Data::AssetBus::Handler::BusConnect(m_bakedHeightfieldAsset.GetId());
        }
    }

    void HeightfieldColliderComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        Physics::CollisionFilteringRequestBus::Handler::BusDisconnect();
        ColliderComponentRequestBus::Handler::BusDisconnect();

        m_heightfieldCollider.reset();
    }

    void HeightfieldColliderComponent::BlockOnPendingJobs()
    {
        if (m_heightfieldCollider)
        {
            m_heightfieldCollider->BlockOnPendingJobs();
        }
    }

    void HeightfieldColliderComponent::SetColliderConfiguration(const Physics::ColliderConfiguration& colliderConfig)
    {
        if (GetEntity()->GetState() == AZ::Entity::State::Active)
        {
            AZ_Warning(
                "PhysX", false, "Trying to call SetShapeConfiguration for entity \"%s\" while entity is active.",
                GetEntity()->GetName().c_str());
            return;
        }
        *m_colliderConfig = colliderConfig;
    }

    void HeightfieldColliderComponent::SetBakedHeightfieldAsset(const AZ::Data::Asset<Pipeline::HeightFieldAsset>& heightfieldAsset)
    {
        m_bakedHeightfieldAsset = heightfieldAsset;
    }

    // ColliderComponentRequestBus
    AzPhysics::ShapeColliderPairList HeightfieldColliderComponent::GetShapeConfigurations()
    {
        AzPhysics::ShapeColliderPairList shapeConfigurationList({ AzPhysics::ShapeColliderPair(m_colliderConfig, m_shapeConfig) });
        return shapeConfigurationList;
    }

    AZStd::shared_ptr<Physics::Shape> HeightfieldColliderComponent::GetHeightfieldShape()
    {
        return m_heightfieldCollider->GetHeightfieldShape();
    }

    // ColliderComponentRequestBus
    AZStd::vector<AZStd::shared_ptr<Physics::Shape>> HeightfieldColliderComponent::GetShapes()
    {
        return { GetHeightfieldShape() };
    }

    // CollisionFilteringRequestBus
    void HeightfieldColliderComponent::SetCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag)
    {
        if (auto heightfield = GetHeightfieldShape())
        {
            if (Physics::Utils::FilterTag(heightfield->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionLayer layer;
                Physics::CollisionRequestBus::BroadcastResult(
                    success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, layer);
                if (success)
                {
                    heightfield->SetCollisionLayer(layer);
                }
            }
        }
    }

    // CollisionFilteringRequestBus
    AZStd::string HeightfieldColliderComponent::GetCollisionLayerName()
    {
        AZStd::string layerName;
        if (auto heightfield = GetHeightfieldShape())
        {
            Physics::CollisionRequestBus::BroadcastResult(
                layerName, &Physics::CollisionRequests::GetCollisionLayerName, heightfield->GetCollisionLayer());
        }
        return layerName;
    }

    // CollisionFilteringRequestBus
    void HeightfieldColliderComponent::SetCollisionGroup(const AZStd::string& groupName, AZ::Crc32 colliderTag)
    {
        if (auto heightfield = GetHeightfieldShape())
        {
            if (Physics::Utils::FilterTag(heightfield->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionGroup group;
                Physics::CollisionRequestBus::BroadcastResult(
                    success, &Physics::CollisionRequests::TryGetCollisionGroupByName, groupName, group);
                if (success)
                {
                    heightfield->SetCollisionGroup(group);
                }
            }
        }
    }

    // CollisionFilteringRequestBus
    AZStd::string HeightfieldColliderComponent::GetCollisionGroupName()
    {
        AZStd::string groupName;
        if (auto heightfield = GetHeightfieldShape())
        {
            Physics::CollisionRequestBus::BroadcastResult(
                groupName, &Physics::CollisionRequests::GetCollisionGroupName, heightfield->GetCollisionGroup());
        }

        return groupName;
    }

    // CollisionFilteringRequestBus
    void HeightfieldColliderComponent::ToggleCollisionLayer(const AZStd::string& layerName, AZ::Crc32 colliderTag, bool enabled)
    {
        if (auto heightfield = GetHeightfieldShape())
        {
            if (Physics::Utils::FilterTag(heightfield->GetTag(), colliderTag))
            {
                bool success = false;
                AzPhysics::CollisionLayer layer;
                Physics::CollisionRequestBus::BroadcastResult(
                    success, &Physics::CollisionRequests::TryGetCollisionLayerByName, layerName, layer);
                if (success)
                {
                    auto group = heightfield->GetCollisionGroup();
                    group.SetLayer(layer, enabled);
                    heightfield->SetCollisionGroup(group);
                }
            }
        }
    }

} // namespace PhysX
