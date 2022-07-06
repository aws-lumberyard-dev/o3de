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

        void PrefabOverrideHandler::RegisterOverridePrefix(AZ::Dom::Path path, AZStd::weak_ptr<AZ::Dom::Value> value)
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
                    TemplateId focusedTemplateId = prefabFocusInterface->GetFocusedPrefabTemplateId(editorEntityContextId);
                    TemplateReference templateRef = prefabSystemComponentInterface->FindTemplate(focusedTemplateId);
                    templateRef->get().RegisterOverridePrefix(path, value);
                }
            }
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
                    TemplateId focusedTemplateId = prefabFocusInterface->GetFocusedPrefabTemplateId(editorEntityContextId);
                    TemplateReference templateRef = prefabSystemComponentInterface->FindTemplate(focusedTemplateId);
                    return templateRef->get().IsOverridePresent(path);
                }
            }
            return false;
        }

        void PrefabOverrideHandler::PrintOverrides()
        {
            /*
        AZStd::vector<AZStd::pair<AZ::Dom::Path, Prefab::PrefabDomValue*>> results;
        auto visitorFn = [&results](const
             * AZ::Dom::Path& path, Prefab::PrefabDomValue* patchValue)
        {
            results.emplace_back(path, patchValue);

             * };

        m_overrideTree.VisitPath(AZ::Dom::Path(), AZ::Dom::PrefixTreeMatch::PathAndSubpaths, visitorFn);

        for
             * (const auto& pair : results)
        {
            // TODO: when the value is null, we should probably clean up the entry
             * from prefix tree as well since it
            // represents a patch that could have been deleted.
            if
             * (pair.second->IsNull() == false)
            {
                Prefab::PrefabDomUtils::PrintPrefabDomValue(

             * AZStd::string::format("Patch value matching key '%s' is ", pair.first.ToString().c_str()), *(pair.second));
            }

             * }*/
        }
    }
}
