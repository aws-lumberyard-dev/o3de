/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

#include <MetaData.h>

namespace PrefabDependencyViewer::Utils
{
    class Node;

    using TemplateId  = AzToolsFramework::Prefab::TemplateId;
    using NodePtr     = AZStd::shared_ptr<Node>;
    using ChildrenList = AZStd::vector<NodePtr>;

    /**
     * Node is a building block for the DirectedGraph. It stores
     * generic MetaData about Prefab or an Asset. It also keeps
     * track of it's parent which helps with adding connections
     * in the GraphCanvas UI.
     */
    class Node
    {
    public:
        enum NodeType
        {
            Prefab = 0,
            Asset
        };

        AZ_CLASS_ALLOCATOR(Node, AZ::SystemAllocator, 0);

        Node(MetaData* metaData, Node* parent)
            : m_metaData(metaData)
            , m_parent(parent)
        {
        }

        void AddChildAndSetParent(NodePtr child)
        {
            child->SetParent(this);
            m_children.push_back(child);
        }

        MetaData* GetMetaData()
        {
            return m_metaData.get();
        }

        Node* GetParent()
        {
            return m_parent;
        }

        void SetParent(Node* parent)
        {
            m_parent = parent;
        }

        ChildrenList& GetChildren()
        {
            return m_children;
        }

        static NodePtr CreatePrefabNode(TemplateId tid, AZStd::string source, Node* parent=nullptr) {
            return AZStd::make_shared<Node>(aznew PrefabMetaData(tid, source), parent);
        }

        static NodePtr CreateAssetNode(AZStd::string asset_description) {
            return AZStd::make_shared<Node>(aznew AssetMetaData(asset_description), nullptr);
        }

    private:
        AZStd::unique_ptr<MetaData> m_metaData;
        Node* m_parent;
        ChildrenList m_children;
    };
} // namespace PrefabDependencyViewer::Utils
