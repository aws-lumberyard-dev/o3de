/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideHandler.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

 namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabOverrideHandler::PrefabOverrideHandler()
        {
            AZ::Interface<PrefabOverrideInterface>::Register(this);
        }

        PrefabOverrideHandler::~PrefabOverrideHandler()
        {
            AZ::Interface<PrefabOverrideInterface>::Unregister(this);
        }

        bool PrefabOverrideHandler::IsOverridePresent(AZ::Dom::Path path)
        {
            PrefabFocusInterface* prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (prefabFocusInterface != nullptr)
            {
                PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
                if (prefabSystemComponentInterface != nullptr)
                {
                    AzFramework::EntityContextId editorEntityContextId;
                    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                        editorEntityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
                    Prefab::InstanceOptionalReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(editorEntityContextId);
                    if (focusedInstance.has_value() && path.size() > 1)
                    {
                        AZStd::string_view overriddenInstanceKey = path[1].GetKey().GetStringView();
                        Prefab::InstanceOptionalReference overriddenInstance =
                            focusedInstance->get().FindNestedInstance(overriddenInstanceKey);
                        if (overriddenInstance.has_value())
                        {
                            auto pathIterator = path.begin();
                            pathIterator++;
                            pathIterator++;
                            AZ::Dom::Path modifiedPath(pathIterator, path.end());
                            LinkReference link = prefabSystemComponentInterface->FindLink(overriddenInstance->get().GetLinkId());
                            if (link.has_value())
                            {
                                return link->get().IsOverridePresent(modifiedPath);
                            }
                        }
                    }
                }
            }
            return false;
        }
    }
}
