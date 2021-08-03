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

        Node(MetaData* metaData, Node* parent);

        void AddChildAndSetParent(NodePtr child);

        const MetaData* GetMetaData() const;

        const Node* GetParent() const;
        void SetParent(Node* parent);

        const ChildrenList& GetChildren() const;

        static NodePtr CreatePrefabNode(TemplateId tid, AZStd::string source, Node* parent = nullptr);
        static NodePtr CreateAssetNode(AZStd::string asset_description);
    private:
        AZStd::unique_ptr<MetaData> m_metaData;
        Node* m_parent;
        ChildrenList m_children;
    };
} // namespace PrefabDependencyViewer::Utils
