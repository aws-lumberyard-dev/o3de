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

#pragma optimize("", off)

namespace RecastNavigation
{
    void EditorRecastNavigationMeshComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorRecastNavigationMeshComponent, BaseClass>()
                ->Field("Auto-update in Editor", &EditorRecastNavigationMeshComponent::m_autoUpdateInEditor)
                ->Version(1);

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<EditorRecastNavigationMeshComponent>("Recast Navigation Mesh",
                    "[Calculates the walkable navigation mesh within a specified area.]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(nullptr, &EditorRecastNavigationMeshComponent::m_autoUpdateInEditor, "Auto-update in Editor",
                        "Automatically calculates and shows the navigation mesh in the Editor viewport.")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorRecastNavigationMeshComponent::OnAutoUpdateChanged)
                    ;

                editContext->Class<RecastNavigationMeshComponentController>(
                    "MeshComponentController", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &RecastNavigationMeshComponentController::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("EditorRecastNavigationMeshComponentTypeId",
                BehaviorConstant(AZ::Uuid(EditorRecastNavigationMeshComponentTypeId)))
                ->Attribute(AZ::Script::Attributes::Module, "navigation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation);
        }
    }

    EditorRecastNavigationMeshComponent::EditorRecastNavigationMeshComponent(const RecastNavigationMeshConfig& config)
        : BaseClass(config)
    {
    }

    void EditorRecastNavigationMeshComponent::Activate()
    {
        BaseClass::Activate();
        OnAutoUpdateChanged();
    }

    void EditorRecastNavigationMeshComponent::Deactivate()
    {
        BaseClass::Deactivate();
    }

    void EditorRecastNavigationMeshComponent::OnAutoUpdateChanged()
    {
        if (m_autoUpdateInEditor)
        {
            m_controller.m_configuration.m_enableDebugDraw = true;
            m_inEditorUpdateTick.Enqueue(AZ::TimeMs{ 1000 }, true);
        }
        else
        {
            m_controller.m_configuration.m_enableDebugDraw = false;
            m_inEditorUpdateTick.RemoveFromQueue();
        }
    }

    void EditorRecastNavigationMeshComponent::OnEditorUpdateTick()
    {
        m_controller.UpdateNavigationMeshAsync();
    }
} // namespace RecastNavigation

#pragma optimize("", on)
