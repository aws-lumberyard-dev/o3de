/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <DirectedGraph.h>

namespace PrefabDependencyViewer::Utils
{
    void DirectedTree::SetRoot(NodePtr root)
    {
        AZ_Assert(!m_root, "Prefab Dependency Viewer - Memory leak in the graph because the root was already set.");
        m_root = root;
    }

    AZStd::shared_ptr<const Node> DirectedTree::GetRoot() const
    {
        return m_root;
    }

    AZStd::tuple<AZStd::vector<int>, int> DirectedTree::CountNodesAtEachLevel() const
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
        return AZStd::make_tuple(AZStd::move(count), widestLevelSize);
    }
}
