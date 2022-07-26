/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplateInterface.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverridePublicHandler.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabOverridePublicHandler::PrefabOverridePublicHandler()
        {
            AZ::Interface<PrefabOverridePublicInterface>::Register(this);
        }

        PrefabOverridePublicHandler::~PrefabOverridePublicHandler()
        {
            AZ::Interface<PrefabOverridePublicInterface>::Unregister(this);
        }

        bool PrefabOverridePublicHandler::IsOverridePresent(AZ::EntityId entityId)
        {
            PrefabSystemComponentInterface* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            if (prefabSystemComponentInterface != nullptr)
            {
                PrefabFocusInterface* prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
                if (prefabFocusInterface == nullptr)
                {
                    return false;
                }

                AzFramework::EntityContextId editorEntityContextId;
                AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                    editorEntityContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);
                Prefab::InstanceOptionalReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(editorEntityContextId);

                auto* instanceToTemplateInterface = AZ::Interface<Prefab::InstanceToTemplateInterface>::Get();
                if (instanceToTemplateInterface == nullptr)
                {
                    return false;
                }
                AZ::Dom::Path absoluteEntityAliasPath = instanceToTemplateInterface->GenerateAbsoluteEntityAliasPath(entityId);

                if (focusedInstance.has_value() && absoluteEntityAliasPath.size() > 1)
                {
                    AZStd::string_view overriddenInstanceKey = absoluteEntityAliasPath[1].GetKey().GetStringView();
                    Prefab::InstanceOptionalReference overriddenInstance = focusedInstance->get().FindNestedInstance(overriddenInstanceKey);
                    if (overriddenInstance.has_value())
                    {
                        auto pathIterator = absoluteEntityAliasPath.begin();
                        pathIterator++;
                        pathIterator++;
                        AZ::Dom::Path modifiedPath(pathIterator, absoluteEntityAliasPath.end());
                        LinkReference link = prefabSystemComponentInterface->FindLink(overriddenInstance->get().GetLinkId());
                        return m_prefabOverrideHandler.IsOverridePresent(modifiedPath, overriddenInstance->get().GetLinkId());
                    }
                }
            }

            return false;
        }
    } // namespace Prefab
} // namespace AzToolsFramework
