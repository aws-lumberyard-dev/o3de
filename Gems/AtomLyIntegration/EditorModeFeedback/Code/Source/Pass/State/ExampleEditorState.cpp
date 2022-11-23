/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/ExampleEditorState.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>

namespace AZ::Render
{
    ExampleEditorState::ExampleEditorState()
        : EditorStateBase(
            EditorState::FocusMode,
            "EditorStateTutorial",
            PassNameList{ AZ::Name("ExampleEffectTemplate") })
    {
    }

    AzToolsFramework::EntityIdList ExampleEditorState::GetMaskedEntities() const
    {
        AzToolsFramework::EntityIdList initialSelectedEntityList, selectedEntityList;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            initialSelectedEntityList, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        // Drill down any entity hierarchies to select all children of the currently selected entities
        for (const auto& selectedEntityId : initialSelectedEntityList)
        {
            AZStd::queue<AZ::EntityId> entityIdQueue;
            entityIdQueue.push(selectedEntityId);

            while (!entityIdQueue.empty())
            {
                AZ::EntityId entityId = entityIdQueue.front();
                entityIdQueue.pop();

                if (entityId.IsValid())
                {
                    selectedEntityList.push_back(entityId);
                }

                AzToolsFramework::EntityIdList children;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                    children, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildren);

                for (AZ::EntityId childEntityId : children)
                {
                    entityIdQueue.push(childEntityId);
                }
            }
        }

        return selectedEntityList;
    }
} // namespace AZ::Render
