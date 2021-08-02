/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Node.h>

namespace PrefabDependencyViewer::Utils
{
    Node::Node(MetaData* metaData, Node* parent)
        : m_metaData(metaData)
        , m_parent(parent)
    {
    }

    void Node::AddChildAndSetParent(NodePtr child)
    {
        child->SetParent(this);
        m_children.push_back(child);
    }

    MetaData* Node::GetMetaData()
    {
        return m_metaData.get();
    }

    Node* Node::GetParent()
    {
        return m_parent;
    }

    void Node::SetParent(Node* parent)
    {
        m_parent = parent;
    }

    ChildrenList& Node::GetChildren()
    {
        return m_children;
    }

    /* static */ NodePtr Node::CreatePrefabNode(TemplateId tid, AZStd::string source, Node* parent)
    {
        return AZStd::make_shared<Node>(aznew PrefabMetaData(tid, source), parent);
    }

    /* static */ NodePtr Node::CreateAssetNode(AZStd::string asset_description)
    {
        return AZStd::make_shared<Node>(aznew AssetMetaData(asset_description), nullptr);
    }
}
