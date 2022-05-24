/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorRecastNavigationMeshComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <Components/RecastNavigationMeshComponent.h>

namespace RecastNavigation
{
    EditorRecastNavigationMeshComponent::EditorRecastNavigationMeshComponent()
        : m_updateEvent([this]() { OnUpdateEvent(); }, AZ::Name("EditorRecastNavigationMeshUpdate"))
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
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_enableAutoUpdateInEditor,
                        "AutoUpdate in Editor", "If enabled, calculates the navigation mesh in the editor viewport (outside of game mode)")
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &EditorRecastNavigationMeshComponent::OnAutoUpdateChanged);
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

        if (m_enableAutoUpdateInEditor)
        {
            m_updateEvent.Enqueue(AZ::TimeMs{ 1000 }, true);
        }
    }

    void EditorRecastNavigationMeshComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
    }

    void EditorRecastNavigationMeshComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<RecastNavigationMeshComponent>(m_meshConfig, m_enableDebugDraw);
    }

    void EditorRecastNavigationMeshComponent::OnUpdateEvent()
    {
    }

    void EditorRecastNavigationMeshComponent::OnAutoUpdateChanged()
    {
        if (m_enableAutoUpdateInEditor)
        {
            m_updateEvent.Enqueue(AZ::TimeMs{ 1000 }, true);
        }
        else
        {
            m_updateEvent.RemoveFromQueue();
        }
    }
} // namespace RecastNavigation
