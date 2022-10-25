/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OutlinerAdapter.h"
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

namespace AZ::DocumentPropertyEditor
{
    OutlinerAdapter::OutlinerAdapter()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusConnect();

        AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

        AzToolsFramework::ContainerEntityNotificationBus::Handler::BusConnect(editorEntityContextId);

        HandleReset();
    }

    OutlinerAdapter::~OutlinerAdapter()
    {
        AzToolsFramework::ContainerEntityNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityInfoNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    void OutlinerAdapter::HandleReset()
    {
        m_rootNode = AZStd::make_unique<OutlinerNode>();

        AZStd::function<void(OutlinerNode& outlinerNode, const AZ::EntityId& entityId)> populateChildren = 
            [&](OutlinerNode& outlinerNode, const AZ::EntityId& entityId)
        {
            AZStd::size_t childCount = 0;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                childCount, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildCount);
            for (size_t childIndex = 0; childIndex < childCount; ++childIndex)
            {
                AZ::EntityId childId;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                    childId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChild, childIndex);

                outlinerNode.m_children.emplace_back(AZStd::make_unique<OutlinerNode>());
                auto* newChild = outlinerNode.m_children.back().get();
                newChild->m_visible = AzToolsFramework::IsEntitySetToBeVisible(childId);
                newChild->m_locked = AzToolsFramework::IsEntityLocked(childId);
                newChild->m_name = AzToolsFramework::GetEntityName(childId);
                populateChildren(*newChild, childId);
            }
        };
        populateChildren(*m_rootNode, AZ::EntityId());
    }

    void OutlinerAdapter::OnContentsChanged(const Dom::Path& path, const Dom::Value& value)
    {
        (void)value;
        (void)path;
    }

    Dom::Value OutlinerAdapter::GenerateContents()
    {
        AdapterBuilder builder;
        builder.BeginAdapter();

        AZStd::function<void(OutlinerNode*)> generateChildren = [&](OutlinerNode* currNode)
        {
            for (const auto& currChild : currNode->m_children)
            {
                builder.BeginRow();
                builder.BeginPropertyEditor<Nodes::OutlinerRow>(Dom::Value());
                builder.Attribute(Nodes::OutlinerRow::EntityName, currChild->m_name);
                builder.Attribute(Nodes::OutlinerRow::Visible, currChild->m_visible);
                builder.Attribute(Nodes::OutlinerRow::Locked, currChild->m_locked);
                builder.EndPropertyEditor();
                generateChildren(currChild.get());
                builder.EndRow();
            }
        };

        for (const auto& currChild : m_rootNode->m_children)
        {
            generateChildren(currChild.get());
        }

        builder.EndAdapter();
        return builder.FinishAndTakeResult();
    }
    
    void OutlinerAdapter::OnEditorEntityDuplicated(const AZ::EntityId& oldEntity, const AZ::EntityId& newEntity)
    {
        (void)oldEntity;
        (void)newEntity;
    }
    
    void OutlinerAdapter::OnContextReset()
    {
    }
    
    void OutlinerAdapter::OnStartPlayInEditorBegin()
    {
    }
    
    void OutlinerAdapter::OnStartPlayInEditor()
    {
    }

    void OutlinerAdapter::OnEntityInfoResetBegin()
    {
    }
    
    void OutlinerAdapter::OnEntityInfoResetEnd()
    {
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedAddChildBegin(AZ::EntityId parentId, AZ::EntityId childId)
    {
        (void)parentId;
        (void)childId;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
    {
        (void)parentId;
        (void)childId;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedRemoveChildBegin(AZ::EntityId parentId, AZ::EntityId childId)
    {
        (void)parentId;
        (void)childId;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
    {
        (void)parentId;
        (void)childId;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedOrderBegin(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index)
    {
        (void)parentId;
        (void)childId;
        (void)index;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedOrderEnd(AZ::EntityId parentId, AZ::EntityId childId, AZ::u64 index)
    {
        (void)parentId;
        (void)childId;
        (void)index;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedSelection(AZ::EntityId entityId, bool selected)
    {
        (void)entityId;
        (void)selected;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedLocked(AZ::EntityId entityId, bool locked)
    {
        (void)entityId;
        (void)locked;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible)
    {
        (void)entityId;
        (void)visible;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name)
    {
        (void)entityId;
        (void)name;
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedUnsavedChanges(AZ::EntityId entityId)
    {
        (void)entityId;
    }
    
    void OutlinerAdapter::OnContainerEntityStatusChanged(AZ::EntityId entityId, bool open)
    {
        (void)entityId;
        (void)open;
    }

} // namespace AZ::DocumentPropertyEditor
