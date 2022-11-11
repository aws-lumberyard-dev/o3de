/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoRevertOverrides.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

 namespace AzToolsFramework
{
    namespace Prefab
    {
        bool PrefabOverrideHandler::AreOverridesPresent(AZ::Dom::Path path, LinkId linkId)
        {
            PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            if (prefabSystemComponentInterface != nullptr)
            {
                LinkReference link = prefabSystemComponentInterface->FindLink(linkId);
                if (link.has_value())
                {
                    return link->get().AreOverridesPresent(path);
                }
            }
            return false;
        }

        void PrefabOverrideHandler::RevertOverrides(AZ::Dom::Path path, LinkId linkId)
        {
            PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            if (prefabSystemComponentInterface != nullptr)
            {
                LinkReference link = prefabSystemComponentInterface->FindLink(linkId);
                if (link.has_value())
                {
                    ScopedUndoBatch undoBatch("Revert Prefab Overrides");

                    PrefabUndoRevertOverrides* state = new Prefab::PrefabUndoRevertOverrides("Capture Override SubTree");
                    auto subTree = link->get().RemoveOverrides(path);
                    state->Capture(path, AZStd::move(subTree), linkId);
                    state->SetParent(undoBatch.GetUndoBatch());
                    
                    link->get().UpdateTarget();
                    prefabSystemComponentInterface->PropagateTemplateChanges(link->get().GetTargetTemplateId());
                    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
                        &AzToolsFramework::ToolsApplicationRequestBus::Events::ClearDirtyEntities);
                }
            }
        }
    }
} // namespace AzToolsFramework
