/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>

#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

#include <MetaData.h>
#include <Node.h>

namespace PrefabDependencyViewer::Utils
{
    using NodePtr = AZStd::shared_ptr<Node>;
    using TemplateId = AzToolsFramework::Prefab::TemplateId;

    /**
     * DirectedGraph is a Graph data structure where edges are directed.
     * It keeps track of node children as well as all the Nodes that
     * have been created. This helps the Graph manage it's own memory.
     * It also contains the root of the tree since it's being used to
     * represent a directed tree with no cycles. It's being used as a
     * generic graph structure to represent a Prefab Dependency Tree. 
     */
    class DirectedTree
    {
    public:
        DirectedTree() = default;
        ~DirectedTree() = default;

        void SetRoot(NodePtr root);
        AZStd::shared_ptr<const Node> GetRoot() const;
        AZStd::tuple<AZStd::vector<int>, int> CountNodesAtEachLevel() const;

    private:
        NodePtr m_root = nullptr;
    };
}
