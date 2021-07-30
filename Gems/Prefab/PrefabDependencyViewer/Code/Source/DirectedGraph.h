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

        void SetRoot(AZStd::shared_ptr<Node> root) {
            if (m_root) {
                AZ_Assert(false, "Prefab Dependency Viewer - Memory leak in the graph because the root was already set.");
            }

            m_root = root;
        }

        AZStd::shared_ptr<Node> GetRoot() const
        {
            return m_root;
        }

        ~DirectedTree() = default;

        AZStd::tuple<AZStd::vector<int>, int> countNodesAtEachLevel() const
        {
            /** Directed Graph can't have cycles because of the
            non-circular nature of the Prefab. */
            int widestLevelSize = 0;
            AZStd::vector<int> count;

            using pair = AZStd::pair<int, Node*>;
            AZStd::queue<pair> queue;
            queue.emplace(0, m_root.get());

            while (!queue.empty())
            {
                auto [level, currNode] = queue.front();
                queue.pop();

                if (count.size() <= level)
                {
                    // Started a new level so check if the previous level
                    // was any bigger then the widest level size seen so far.
                    int prevLevelCount = level != 0 ? count[level - 1] : 0;
                    widestLevelSize = AZStd::max(widestLevelSize, prevLevelCount);
                    count.push_back(1);
                }
                else
                {
                    ++count[level];
                }

                auto children = currNode->GetChildren();
                for (const NodePtr& node : children)
                {
                    queue.emplace(level + 1, node.get());
                }
            }

            // Check if the last level was the widest.
            widestLevelSize = AZStd::max(widestLevelSize, count[count.size() - 1]);
            return AZStd::make_tuple(count, widestLevelSize);
        }
    private:
        NodePtr m_root = nullptr;
    };
}
