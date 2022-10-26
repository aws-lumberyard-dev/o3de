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
        // TRYAN - Doing this for now to keep things simple, but curious if it's better to reuse our map and only update values for
        // entities that we're already tracking
        m_entityNodeCache.clear();

        m_rootNode = AZStd::make_unique<OutlinerNode>();

        AZStd::function<void(OutlinerNode& parentNode, const AZ::EntityId& entityId)> populateChildren = 
            [&](OutlinerNode& parentNode, const AZ::EntityId& entityId)
        {
            AZStd::size_t childCount = 0;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                childCount, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChildCount);
            for (size_t childIndex = 0; childIndex < childCount; ++childIndex)
            {
                AZ::EntityId childId;
                AzToolsFramework::EditorEntityInfoRequestBus::EventResult(
                    childId, entityId, &AzToolsFramework::EditorEntityInfoRequestBus::Events::GetChild, childIndex);

                parentNode.m_children.emplace_back(AZStd::make_unique<OutlinerNode>());
                auto* newChild = parentNode.m_children.back().get();
                newChild->m_visible = AzToolsFramework::IsEntitySetToBeVisible(childId);
                newChild->m_locked = AzToolsFramework::IsEntityLocked(childId);
                newChild->m_name = AzToolsFramework::GetEntityName(childId);
                newChild->m_parent = &parentNode;
                populateChildren(*newChild, childId);
                m_entityNodeCache[childId] = newChild;
            }
        };

        populateChildren(*m_rootNode, AZ::EntityId());
        m_entityNodeCache[EntityId()] = m_rootNode.get();
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
        // TRYAN - This appears to be expanding the view when an entity is duplicated; I don't think we're ready to handle that yet
    }
    
    void OutlinerAdapter::OnContextReset()
    {
        HandleReset();
        NotifyResetDocument();
    }
    
    void OutlinerAdapter::OnStartPlayInEditorBegin()
    {
        // TRYAN - Might be unnecessary?
    }
    
    void OutlinerAdapter::OnStartPlayInEditor()
    {
        // TRYAN - Might be unnecessary?
    }

    void OutlinerAdapter::OnEntityInfoResetBegin()
    {
        // TRYAN - Might not be anything to do here
    }
    
    void OutlinerAdapter::OnEntityInfoResetEnd()
    {
        HandleReset();
        NotifyResetDocument();
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedAddChildBegin(AZ::EntityId parentId, AZ::EntityId childId)
    {
        (void)parentId;
        (void)childId;
        // TRYAN - Might not be anything to do here, unless we end up having to queue up incoming updates while
        //         something else is happening
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedAddChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
    {
        if (!m_entityNodeCache.contains(parentId))
        {
            AZ_Warning("OutlinerAdapter", false, "Child EntityId was added to Outliner before its parent!");
            return;
        }

        OutlinerNode* parentNode = m_entityNodeCache[parentId];
        parentNode->m_children.emplace_back(AZStd::make_unique<OutlinerNode>());
        OutlinerNode* newChild = parentNode->m_children.back().get();

        // If this is the first time we're hearing of this entity then cache it
        if (!m_entityNodeCache.contains(childId))
        {
            newChild->m_visible = AzToolsFramework::IsEntitySetToBeVisible(childId);
            newChild->m_locked = AzToolsFramework::IsEntityLocked(childId);
            newChild->m_name = AzToolsFramework::GetEntityName(childId);
            m_entityNodeCache[childId] = newChild;
        }
        else // This must be a node the Outliner already knew about so we need to move it
        {
            OutlinerNode* childNode = m_entityNodeCache[childId];
            newChild->m_visible = childNode->m_visible;
            newChild->m_locked = childNode->m_locked;
            newChild->m_name = childNode->m_name;

            OutlinerNode* oldParentNode = childNode->m_parent;
            if (oldParentNode)
            {
                for (auto iter = oldParentNode->m_children.begin(); iter != oldParentNode->m_children.end(); ++iter)
                {
                    if (iter->get() == childNode)
                    {
                        oldParentNode->m_children.erase(iter);
                        break;
                    }
                }
            }
        }

        NotifyResetDocument();
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedRemoveChildBegin(AZ::EntityId parentId, AZ::EntityId childId)
    {
        (void)parentId;
        (void)childId;
        // TRYAN - Might not be anything to do here
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedRemoveChildEnd(AZ::EntityId parentId, AZ::EntityId childId)
    {
        if (!m_entityNodeCache.contains(parentId) || !m_entityNodeCache.contains(childId))
        {
            AZ_Warning("OutlinerAdapter", false, "Received remove notification for untracked child or parent node!");
            return;
        }

        OutlinerNode* parentNode = m_entityNodeCache[parentId];
        OutlinerNode* childNode = m_entityNodeCache[childId];

        for (auto iter = parentNode->m_children.begin(); iter != parentNode->m_children.end(); ++iter)
        {
            if (iter->get() == childNode)
            {
                parentNode->m_children.erase(iter);
                break;
            }
        }

        m_entityNodeCache.erase(childId);

        NotifyResetDocument();
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
        if (m_entityNodeCache[entityId])
        {
            AZ_Warning("OutlinerAdapter", false, "Selected EntityId %d is cached! Huzzah!", AZ::u64(entityId));
        }
        else
        {
            AZ_Warning("OutlinerAdapter", false, "EntityId %d was not cached when updating Selection state!", AZ::u64(entityId));
        }
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedLocked(AZ::EntityId entityId, bool locked)
    {
        if (m_entityNodeCache[entityId])
        {
            m_entityNodeCache[entityId]->m_locked = locked;
            NotifyResetDocument();
        }
        else
        {
            AZ_Warning("OutlinerAdapter", false, "EntityId %d was not cached when updating Locked state!", AZ::u64(entityId));
        }
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedVisibility(AZ::EntityId entityId, bool visible)
    {
        if (m_entityNodeCache[entityId])
        {
            m_entityNodeCache[entityId]->m_visible = visible;
            NotifyResetDocument();
        }
        else
        {
            AZ_Warning("OutlinerAdapter", false, "EntityId %d was not cached when updating Visibility state!", AZ::u64(entityId));
        }
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name)
    {
        if (m_entityNodeCache[entityId])
        {
            // TRYAN - Note that the current outliner has a case for scrolling the view list to the entity if its selected

            m_entityNodeCache[entityId]->m_name = name;
            NotifyResetDocument();
        }
        else
        {
            AZ_Warning("OutlinerAdapter", false, "EntityId %d was not cached when updating its Name!", AZ::u64(entityId));
        }
    }
    
    void OutlinerAdapter::OnEntityInfoUpdatedUnsavedChanges(AZ::EntityId entityId)
    {
        (void)entityId;
        HandleReset();
        NotifyResetDocument();
    }
    
    void OutlinerAdapter::OnContainerEntityStatusChanged(AZ::EntityId entityId, bool open)
    {
        (void)entityId;
        (void)open;
        HandleReset();
        NotifyResetDocument();
    }
} // namespace AZ::DocumentPropertyEditor
