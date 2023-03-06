/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabComponentEditPatcher.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyEdit.h>
#include <AzToolsFramework/Prefab/Undo/PrefabUndoComponentPropertyOverride.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework::Prefab
{
    bool PrefabComponentEditPatcher::CreateAndApplyComponentEditPatch(
        AZStd::string_view relativePathFromOwningPrefab,
        const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo,
        AZ::EntityId entityId)
    {
        if (!propertyChangeInfo.newValue.IsOpaqueValue())
        {
            auto convertToRapidJsonOutcome = AZ::Dom::Json::WriteToRapidJsonDocument(
                [propertyChangeInfo](AZ::Dom::Visitor& visitor)
                {
                    const bool copyStrings = false;
                    return propertyChangeInfo.newValue.Accept(visitor, copyStrings);
                });

            if (!convertToRapidJsonOutcome.IsSuccess())
            {
                AZ_Assert(false, "PrefabDom value converted from AZ::Dom::Value.");
                return false;
            }
            else
            {

                auto owningInstance = AZ::Interface<InstanceEntityMapperInterface>::Get()->FindOwningInstance(entityId);
                if (!owningInstance.has_value())
                {
                    AZ_Assert(false, "Entity owning the component doesn't have an owning prefab instance.");
                    return false;
                }

                auto prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

                if (prefabSystemComponentInterface != nullptr)
                {
                    AZ_Assert(false, "PrefabSystemComponentInterface is not found.");
                    return false;
                }
                

                const PrefabDom& templateDom = prefabSystemComponentInterface->FindTemplateDom(owningInstance->get().GetTemplateId());
                PrefabDomPath prefabDomPathToComponentProperty(relativePathFromOwningPrefab.data()); 
                const PrefabDomValue* beforeValueOfComponentProperty = prefabDomPathToComponentProperty.Get(templateDom);

                AzToolsFramework::Prefab::PrefabDom afterValueOfComponentProperty = convertToRapidJsonOutcome.TakeValue();
                ScopedUndoBatch undoBatch("Update component in a prefab template");
                PrefabUndoComponentPropertyEdit* state = aznew PrefabUndoComponentPropertyEdit("Undo Updating Component");
                state->SetParent(undoBatch.GetUndoBatch());
                state->Capture(*beforeValueOfComponentProperty, afterValueOfComponentProperty, entityId, relativePathFromOwningPrefab);
                state->Redo();

                return true;
            }
        }
        else
        {
            AZ_Assert(
                false, "Opaque property encountered in PrefabAdapter::GeneratePropertyEditPatch. It should have been a serialized value.");
            return false;
        }
    }

    bool PrefabComponentEditPatcher::CreateAndApplyComponentOverridePatch(
        AZ::Dom::Path relativePathFromOwningPrefab,
        const AZ::DocumentPropertyEditor::ReflectionAdapter::PropertyChangeInfo& propertyChangeInfo,
        AZ::EntityId entityId)
    {
        if (!propertyChangeInfo.newValue.IsOpaqueValue())
        {
            auto convertToRapidJsonOutcome = AZ::Dom::Json::WriteToRapidJsonDocument(
                [propertyChangeInfo](AZ::Dom::Visitor& visitor)
                {
                    const bool copyStrings = false;
                    return propertyChangeInfo.newValue.Accept(visitor, copyStrings);
                });

            if (!convertToRapidJsonOutcome.IsSuccess())
            {
                AZ_Assert(false, "PrefabDom value converted from AZ::Dom::Value.");
                return false;
            }
            else
            {
                auto owningInstance = AZ::Interface<InstanceEntityMapperInterface>::Get()->FindOwningInstance(entityId);
                if (!owningInstance.has_value())
                {
                    AZ_Assert(false, "Entity owning the component doesn't have an owning prefab instance.");
                    return false;
                }

                AzToolsFramework::Prefab::PrefabDom afterValueOfComponentProperty = convertToRapidJsonOutcome.TakeValue();
                ScopedUndoBatch undoBatch("override a component in a nested prefab template");
                PrefabUndoComponentPropertyOverride* state = aznew PrefabUndoComponentPropertyOverride("Undo overriding Component");
                state->SetParent(undoBatch.GetUndoBatch());
                state->CaptureAndRedo(owningInstance->get(), relativePathFromOwningPrefab, afterValueOfComponentProperty);

                return true;
            }
        }
        else
        {
            AZ_Assert(
                false, "Opaque property encountered in PrefabAdapter::GeneratePropertyEditPatch. It should have been a serialized value.");
            return false;
        }
    }
}
