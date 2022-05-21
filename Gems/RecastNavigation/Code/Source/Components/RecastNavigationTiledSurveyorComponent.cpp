/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RecastNavigationTiledSurveyorComponent.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <DebugDraw/DebugDrawBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

AZ_CVAR(
    bool, cl_navmesh_showInputData, false, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, draws triangle mesh input data that was used for the navigation mesh calculation");
AZ_CVAR(
    float, cl_navmesh_showInputDataSeconds, 30.f, nullptr, AZ::ConsoleFunctorFlags::Null,
    "If enabled, keeps the debug triangle mesh input for the specified number of seconds");

AZ_DECLARE_BUDGET(Navigation);

namespace RecastNavigation
{
    RecastNavigationTiledSurveyorComponent::RecastNavigationTiledSurveyorComponent(bool debugDrawInputData)
        : m_debugDrawInputData(debugDrawInputData)
    {
    }

    void RecastNavigationTiledSurveyorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<RecastNavigationTiledSurveyorComponent, AZ::Component>()
                ->Version(1)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RecastNavigationTiledSurveyorComponent>()->RequestBus("RecastNavigationSurveyorRequestBus");
        }
    }

    void RecastNavigationTiledSurveyorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationTiledSurveyorComponent"));
        provided.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void RecastNavigationTiledSurveyorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationTiledSurveyorComponent"));
        incompatible.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void RecastNavigationTiledSurveyorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("BoxShapeService"));
    }

    void RecastNavigationTiledSurveyorComponent::CollectGeometryWithinVolume(const AZ::Aabb& volume, QueryHits& overlapHits)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometryWithinVolume");

        AZ::Vector3 dimension = volume.GetExtents();
        AZ::Transform pose = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), volume.GetCenter());

        Physics::BoxShapeConfiguration shapeConfiguration;
        shapeConfiguration.m_dimensions = dimension;

        AzPhysics::OverlapRequest request = AzPhysics::OverlapRequestHelpers::CreateBoxOverlapRequest(dimension, pose, nullptr);
        request.m_queryType = AzPhysics::SceneQuery::QueryType::Static;
        request.m_collisionGroup = AzPhysics::CollisionGroup::All;

        AzPhysics::SceneQuery::UnboundedOverlapHitCallback unboundedOverlapHitCallback =
            [&overlapHits](AZStd::optional<AzPhysics::SceneQueryHit>&& hit)
        {
            if (hit && ((hit->m_resultFlags & AzPhysics::SceneQuery::EntityId) != 0))
            {
                const AzPhysics::SceneQueryHit& sceneQueryHit = *hit;
                overlapHits.push_back(sceneQueryHit);
            }

            return true;
        };

        request.m_unboundedOverlapHitCallback = unboundedOverlapHitCallback;

        if (auto sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);

            // Note: blocking call
            sceneInterface->QueryScene(sceneHandle, &request);
            // results are in overlapHits
        }
    }

    void RecastNavigationTiledSurveyorComponent::AppendColliderGeometry(
        TileGeometry& geometry,
        const QueryHits& overlapHits)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: AppendColliderGeometry");

        AZStd::vector<AZ::Vector3> vertices;
        AZStd::vector<AZ::u32> indices;
        AZStd::size_t indicesCount = geometry.m_indices.size();

        AzPhysics::SceneInterface* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);

        for (const auto& overlapHit : overlapHits)
        {
            AzPhysics::SimulatedBody* body = sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, overlapHit.m_bodyHandle);
            if (!body)
            {
                continue;
            }

            AZ::Transform t = AZ::Transform::CreateFromQuaternionAndTranslation(body->GetOrientation(), body->GetPosition());
            overlapHit.m_shape->GetGeometry(vertices, indices, nullptr);

            if (!vertices.empty())
            {
                if (indices.empty())
                {
                    AZStd::vector<AZ::Vector3> transformed;

                    int currentLocalIndex = 0;
                    for (const AZ::Vector3& vertex : vertices)
                    {
                        const AZ::Vector3 translated = t.TransformPoint(vertex);
                        geometry.m_vertices.push_back(RecastVector3(translated));

                        if (cl_navmesh_showInputData || m_debugDrawInputData)
                        {
                            transformed.push_back(translated);
                        }

                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + currentLocalIndex));
                        currentLocalIndex++;
                    }

                    if (cl_navmesh_showInputData || m_debugDrawInputData)
                    {
                        for (size_t i = 2; i < vertices.size(); i += 3)
                        {
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[i - 2], transformed[i - 1], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[i - 1], transformed[i - 0], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[i - 0], transformed[i - 2], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                        }
                    }
                }
                else
                {
                    AZStd::vector<AZ::Vector3> transformed;

                    for (const AZ::Vector3& vertex : vertices)
                    {
                        const AZ::Vector3 translated = t.TransformPoint(vertex);
                        geometry.m_vertices.push_back(RecastVector3(translated));

                        if (cl_navmesh_showInputData || m_debugDrawInputData)
                        {
                            transformed.push_back(translated);
                        }
                    }

                    for (size_t i = 2; i < indices.size(); i += 3)
                    {
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i]));
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i - 1]));
                        geometry.m_indices.push_back(aznumeric_cast<AZ::u32>(indicesCount + indices[i - 2]));
                    }

                    if (cl_navmesh_showInputData || m_debugDrawInputData)
                    {
                        for (size_t i = 2; i < indices.size(); i += 3)
                        {
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[indices[i - 2]], transformed[indices[i - 1]], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[indices[i - 1]], transformed[indices[i - 0]], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                            DebugDraw::DebugDrawRequestBus::Broadcast(&DebugDraw::DebugDrawRequests::DrawLineLocationToLocation,
                                transformed[indices[i - 0]], transformed[indices[i - 2]], AZ::Colors::Red, cl_navmesh_showInputDataSeconds);
                        }
                    }
                }

                indicesCount += vertices.size();
                vertices.clear();
                indices.clear();
            }
        }
    }

    void RecastNavigationTiledSurveyorComponent::Activate()
    {
        AZ::Vector3 position = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(position, GetEntityId(), &AZ::TransformBus::Events::GetWorldTranslation);

        RecastNavigationSurveyorRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RecastNavigationTiledSurveyorComponent::Deactivate()
    {
        RecastNavigationSurveyorRequestBus::Handler::BusDisconnect();
    }

    AZStd::vector<AZStd::shared_ptr<TileGeometry>> RecastNavigationTiledSurveyorComponent::CollectGeometry(
        float tileSize, float borderSize)
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometry");

        AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        const AZ::Vector3& worldMin = worldVolume.GetMin();
        const AZ::Vector3& worldMax = worldVolume.GetMax();

        const AZ::Vector3 border = AZ::Vector3::CreateOne() * borderSize;

        for (int y = 0; y < tilesAlongY; ++y)
        {
            for (int x = 0; x < tilesAlongX; ++x)
            {
                const AZ::Vector3 tileMin{
                    worldMin.GetX() + aznumeric_cast<float>(x) * tileSize,
                    worldMin.GetY() + aznumeric_cast<float>(y) * tileSize,
                    worldMin.GetZ()
                };

                const AZ::Vector3 tileMax{
                    worldMin.GetX() + aznumeric_cast<float>(x + 1) * tileSize,
                    worldMin.GetY() + aznumeric_cast<float>(y + 1) * tileSize,
                    worldMax.GetZ()
                };

                AZ::Aabb tileVolume = AZ::Aabb::CreateFromMinMax(tileMin, tileMax);
                AZ::Aabb scanVolume = AZ::Aabb::CreateFromMinMax(tileMin - border, tileMax + border);

                QueryHits results;
                CollectGeometryWithinVolume(scanVolume, results);

                AZStd::shared_ptr<TileGeometry> geometryData = AZStd::make_unique<TileGeometry>();
                geometryData->m_worldBounds = tileVolume;
                AppendColliderGeometry(*geometryData, results);

                geometryData->m_tileX = x;
                geometryData->m_tileY = y;
                tiles.push_back(geometryData);
            }
        }

        return tiles;
    }

    void RecastNavigationTiledSurveyorComponent::CollectGeometryAsync(
        float tileSize,
        float borderSize,
        AZStd::function<void(AZStd::shared_ptr<TileGeometry>)> tileCallback)
    {
        if (!m_taskGraphEvent || m_taskGraphEvent->IsSignaled())
        {
            AZ_PROFILE_SCOPE(Navigation, "Navigation: CollectGeometryAsync");

            m_taskGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>();
            m_taskGraph.Reset();

            AZStd::vector<AZStd::shared_ptr<TileGeometry>> tiles;

            const AZ::Aabb worldVolume = GetWorldBounds();

            const AZ::Vector3 extents = worldVolume.GetExtents();
            int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
            int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

            const AZ::Vector3& worldMin = worldVolume.GetMin();
            const AZ::Vector3& worldMax = worldVolume.GetMax();

            const AZ::Vector3 border = AZ::Vector3::CreateOne() * borderSize;

            AZStd::vector<AZ::TaskToken*> tileTaskTokens;

            for (int y = 0; y < tilesAlongY; ++y)
            {
                for (int x = 0; x < tilesAlongX; ++x)
                {
                    const AZ::Vector3 tileMin{
                        worldMin.GetX() + aznumeric_cast<float>(x) * tileSize,
                        worldMin.GetY() + aznumeric_cast<float>(y) * tileSize,
                        worldMin.GetZ()
                    };

                    const AZ::Vector3 tileMax{
                        worldMin.GetX() + aznumeric_cast<float>(x + 1) * tileSize,
                        worldMin.GetY() + aznumeric_cast<float>(y + 1) * tileSize,
                        worldMax.GetZ()
                    };

                    AZ::Aabb tileVolume = AZ::Aabb::CreateFromMinMax(tileMin, tileMax);
                    AZ::Aabb scanVolume = AZ::Aabb::CreateFromMinMax(tileMin - border, tileMax + border);
                    AZStd::shared_ptr<TileGeometry> geometryData = AZStd::make_unique<TileGeometry>();
                    geometryData->m_tileCallback = tileCallback;
                    geometryData->m_worldBounds = tileVolume;
                    geometryData->m_scanBounds = scanVolume;
                    geometryData->m_tileX = x;
                    geometryData->m_tileY = y;

                    AZ::TaskToken token = m_taskGraph.AddTask(
                        m_taskDescriptor, [this, geometryData]()
                        {
                            AZ_PROFILE_SCOPE(Navigation, "Navigation: task - computing tile");
                            QueryHits results;
                            CollectGeometryWithinVolume(geometryData->m_scanBounds, results);
                            AppendColliderGeometry(*geometryData, results);

                            geometryData->m_tileCallback(geometryData);
                        });

                    tileTaskTokens.push_back(&token);
                }
            }

            AZ::TaskToken finishToken = m_taskGraph.AddTask(
                m_taskDescriptor, [tileCallback]()
                {
                    tileCallback({});
                });

            for (AZ::TaskToken* task : tileTaskTokens)
            {
                task->Precedes(finishToken);
            }

            m_taskGraph.SubmitOnExecutor(m_taskExecutor, m_taskGraphEvent.get());
        }
    }

    AZ::Aabb RecastNavigationTiledSurveyorComponent::GetWorldBounds() const
    {
        AZ::Aabb worldBounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(worldBounds, GetEntityId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
        return worldBounds;
    }

    int RecastNavigationTiledSurveyorComponent::GetNumberOfTiles(float tileSize) const
    {
        const AZ::Aabb worldVolume = GetWorldBounds();

        const AZ::Vector3 extents = worldVolume.GetExtents();
        const int tilesAlongX = static_cast<int>(AZStd::ceilf(extents.GetX() / tileSize));
        const int tilesAlongY = static_cast<int>(AZStd::ceilf(extents.GetY() / tileSize));

        return tilesAlongX * tilesAlongY;
    }
} // namespace RecastNavigation
