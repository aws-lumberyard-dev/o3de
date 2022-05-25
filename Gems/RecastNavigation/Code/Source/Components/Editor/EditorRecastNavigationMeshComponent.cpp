/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationMeshComponent.h"

#include <DetourDebugDraw.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <Components/RecastNavigationMeshComponent.h>
#include <RecastNavigation/RecastNavigationSurveyorBus.h>

AZ_DECLARE_BUDGET(Navigation);

#pragma optimize("", off)

namespace RecastNavigation
{
    EditorRecastNavigationMeshComponent::EditorRecastNavigationMeshComponent()
        : m_debugDrawEvent(
              [this]()
              {
                  OnDebugDrawTick();
              }, AZ::Name("EditorRecastNavigationDebugViewTick"))
          , m_updateEvent(
              [this]()
              {
                  OnUpdateEvent();
              }, AZ::Name("EditorRecastNavigationMeshUpdate"))
    {
    }

    void EditorRecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        if (const auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationMeshComponent, AZ::Component>()
                ->Field("Configuration", &EditorRecastNavigationMeshComponent::m_meshConfig)
                ->Field("Debug Draw", &EditorRecastNavigationMeshComponent::m_enableDebugDraw)
                ->Field("AutoUpdate in Editor", &EditorRecastNavigationMeshComponent::m_enableAutoUpdateInEditor)
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorRecastNavigationMeshComponent>("Recast Navigation Mesh",
                    "[Calculates the walkable navigation mesh within a specified area.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_meshConfig,
                        "Configuration", "Navigation Mesh configuration")

                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_enableDebugDraw,
                        "Debug Draw", "If enabled, draw the navigation mesh")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshComponent::OnDebugDrawChanged)

                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_enableAutoUpdateInEditor,
                        "AutoUpdate in Editor", "If enabled, calculates the navigation mesh in the editor viewport (outside of game mode)")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshComponent::OnAutoUpdateChanged);
                ;
            }
        }
    }

    void EditorRecastNavigationMeshComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void EditorRecastNavigationMeshComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("RecastNavigationMeshComponent"));
    }

    void EditorRecastNavigationMeshComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RecastNavigationSurveyorService"));
    }

    void EditorRecastNavigationMeshComponent::Activate()
    {
        EditorComponentBase::Activate();

        if (m_enableDebugDraw)
        {
            OnDebugDrawChanged();
        }
        if (m_enableAutoUpdateInEditor)
        {
            OnAutoUpdateChanged();
        }
    }

    void EditorRecastNavigationMeshComponent::Deactivate()
    {
        m_updateEvent.RemoveFromQueue();
        m_debugDrawEvent.RemoveFromQueue();

        EditorComponentBase::Deactivate();
    }

    void EditorRecastNavigationMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationMeshComponent>(m_meshConfig, m_enableDebugDraw);
    }

    void EditorRecastNavigationMeshComponent::OnDebugDrawTick()
    {
        if (m_navObjects)
        {
            AZStd::lock_guard loc(m_navObjects->m_mutex);

            if (m_navObjects->m_mesh)
            {
                duDebugDrawNavMesh(&m_customDebugDraw, *m_navObjects->m_mesh, DU_DRAWNAVMESH_COLOR_TILES);
            }
        }
    }

    void EditorRecastNavigationMeshComponent::OnDebugDrawChanged()
    {
        if (m_enableDebugDraw)
        {
            m_debugDrawEvent.Enqueue(AZ::TimeMs{ 0 }, true);
        }
        else
        {
            m_debugDrawEvent.RemoveFromQueue();
        }
    }

    void EditorRecastNavigationMeshComponent::OnUpdateEvent()
    {
        AZ_PROFILE_SCOPE(Navigation, "Navigation: (Editor) UpdateNavigationMeshAsync");

        if (m_isUpdating == false)
        {
            m_isUpdating = true;

            RecastNavigationSurveyorRequestBus::Event(GetEntityId(),
                &RecastNavigationSurveyorRequests::CollectGeometryAsync,
                m_meshConfig.m_tileSize, aznumeric_cast<float>(m_meshConfig.m_borderSize) * m_meshConfig.m_cellSize,
                [this](AZStd::shared_ptr<TileGeometry> tile)
                {
                    if (tile)
                    {
                        if (tile->IsEmpty())
                        {
                            return;
                        }

                        NavigationTileData navigationTileData = CreateNavigationTile(tile.get(),
                            m_meshConfig, m_context.get());

                        if (navigationTileData.IsValid())
                        {
                            AZ_PROFILE_SCOPE(Navigation, "Navigation: (Editor) UpdateNavigationMeshAsync - tile callback");
                            AZStd::lock_guard lock(m_navObjects->m_mutex);

                            if (const dtTileRef tileRef = m_navObjects->m_mesh->getTileRefAt(tile->m_tileX, tile->m_tileY, 0))
                            {
                                m_navObjects->m_mesh->removeTile(tileRef, nullptr, nullptr);
                            }
                            if (navigationTileData.IsValid())
                            {
                                AttachNavigationTileToMesh(navigationTileData);
                            }
                        }
                    }
                    else
                    {
                        RecastNavigationMeshNotificationBus::Event(GetEntityId(),
                            &RecastNavigationMeshNotifications::OnNavigationMeshUpdated, GetEntityId());
                        m_isUpdating = false;
                    }
                });
        }
    }

    void EditorRecastNavigationMeshComponent::OnAutoUpdateChanged()
    {
        if (m_enableAutoUpdateInEditor)
        {
            CreateEditorNavigationMesh();
            m_updateEvent.Enqueue(AZ::TimeMs{ 1000 }, true);
        }
        else
        {
            m_updateEvent.RemoveFromQueue();
        }
    }

    void EditorRecastNavigationMeshComponent::CreateEditorNavigationMesh()
    {
        if (!m_context)
        {
            m_context = AZStd::make_unique<rcContext>();
        }
        if (!m_navObjects)
        {
            CreateNavigationMesh(GetEntityId(), m_meshConfig.m_tileSize);
        }
    }
} // namespace RecastNavigation

#pragma optimize("", on)
