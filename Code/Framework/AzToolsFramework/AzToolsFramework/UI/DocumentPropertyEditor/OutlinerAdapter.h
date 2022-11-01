/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>

namespace AZ::DocumentPropertyEditor
{
    //! An adapter for displaying an editable list of CVars registered to the console instance.
    //! Supports editing CVars with primitive types, string types, and numeric vector types
    //! (VectorX, Quaternion, and Color).
    class OutlinerAdapter : public DocumentAdapter
        , private AzToolsFramework::ContainerEntityNotificationBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
        , private AzToolsFramework::EditorEntityInfoNotificationBus::Handler
    {
    public:
        OutlinerAdapter();
        virtual ~OutlinerAdapter();

        void HandleReset();

        void OnContentsChanged(const Dom::Path& path, const Dom::Value& value);

    protected:
        Dom::Value GenerateContents() override;

        //! Editor entity context notification bus
        void OnEditorEntityDuplicated(const AZ::EntityId& oldEntity, const AZ::EntityId& newEntity) override;
        void OnContextReset() override;
        void OnStartPlayInEditorBegin() override;
        void OnStartPlayInEditor() override;

        //! EditorEntityInfoNotificationBus::Handler
        //! Get notifications when the EditorEntityInfo changes so we can update our model
        void OnEntityInfoResetBegin() override;
        void OnEntityInfoResetEnd() override;
        void OnEntityInfoUpdatedAddChildBegin(AZ::EntityId parentId, AZ::EntityId childId) override;
        void OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId) override;
        void OnEntityInfoUpdatedRemoveChildBegin(AZ::EntityId parentId, AZ::EntityId childId) override;
        void OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId parentId, AZ::EntityId childId) override;
        void OnEntityInfoUpdatedOrderBegin(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index) override;
        void OnEntityInfoUpdatedOrderEnd(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index) override;
        void OnEntityInfoUpdatedSelection(AZ::EntityId entityId, bool selected) override;
        void OnEntityInfoUpdatedLocked(AZ::EntityId entityId, bool locked) override;
        void OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible) override;
        void OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name) override;
        void OnEntityInfoUpdatedUnsavedChanges(AZ::EntityId entityId) override;

        // ContainerEntityNotificationBus overrides ...
        void OnContainerEntityStatusChanged(AZ::EntityId entityId, bool open) override;

        struct OutlinerNode
        {
            AZStd::string m_name;
            bool m_visible;
            bool m_locked;
            bool m_selected;
            EntityId m_entityId;

            OutlinerNode* m_parent = nullptr;
            AZStd::vector<AZStd::unique_ptr<OutlinerNode>> m_children;
        };

        AZStd::unique_ptr<OutlinerNode> m_rootNode;
        AZStd::unordered_map<EntityId, OutlinerNode*> m_entityNodeCache;
    };
}
