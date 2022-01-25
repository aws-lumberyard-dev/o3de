/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#pragma optimize("", off)
#pragma inline_depth(0)

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/MathIntrinsics.h>
#include <AzCore/std/containers/span.h>

namespace Terrain
{
    template <typename T>
    class QuadTree
    {
    public:
        
        using BitHolderType = uint64_t;

        enum class ChildIndex : uint8_t
        {
            TL = 0b0001,
            TR = 0b0010,
            BL = 0b0100,
            BR = 0b1000,
        };
        
        QuadTree(float physicalSize);
        ~QuadTree() = default;

        void ResizeDepth(uint32_t depth);

        T& GetRoot();
        const T& GetRoot() const;
        T& GetRootAndEnable();

        T& GetChild(const T& node, ChildIndex childIndex);
        const T& GetChild(const T& node, ChildIndex childIndex) const;
        T& GetChildAndEnable(const T& node, ChildIndex childIndex);

        T& GetParent(const T& node);
        const T& GetParent(const T& node) const;

        ChildIndex GetEnabledChildrenIndices(const T& node) const;
        uint32_t GetDepth(const T& node) const;

        void EnableNode(const T& node);
        void DisableNode(const T& node);
        bool IsNodeEnabled(const T& node) const;

        T& GetNodeAtPosition(float x, float y, uint32_t maxDepth = AZStd::numeric_limits<uint32_t>::max());

        const AZStd::vector<T>& GetTreeData() const;
        const AZStd::vector<BitHolderType>& GetTreeEnabledBits() const;

        static bool ChildIsEnabled(ChildIndex bitFlags, ChildIndex specificChild);

    private:

        uint32_t GetIndex(const T& node) const;
        uint32_t GetChildIndex(uint32_t index, ChildIndex childIndex) const;

        void EnableNodeIndex(uint32_t index);
        void DisableNodeIndex(uint32_t index);
        bool IsNodeIndexEnabled(uint32_t index) const;

        uint32_t NumNodesNeededForDepth(uint32_t level) const;
        uint32_t GetDepthFromIndex(uint32_t index) const;

        //! The actual data in the quad tree
        AZStd::vector<T> m_treeData;

        //! Bits representing if a node exists
        static constexpr size_t BitsPerElement = sizeof(BitHolderType) * 8;
        AZStd::vector<BitHolderType> m_treeEnabledBits;

        float m_physicalSize;
        float m_rcpPhysicalSize;

    };

    template <typename T>
    QuadTree<T>::QuadTree(float physicalSize)
        : m_physicalSize(physicalSize)
        , m_rcpPhysicalSize(1.0f / physicalSize)
    {
        // Always be big enough for level 0, i.e., the root node.
        ResizeDepth(0);
    }

    template <typename T>
    void QuadTree<T>::ResizeDepth(uint32_t depth)
    {
        uint32_t nodesNeeded = NumNodesNeededForDepth(depth);
        m_treeData.resize(nodesNeeded);
        m_treeEnabledBits.resize(((nodesNeeded - 1) / BitsPerElement) + 1); // uint64_t holds flags for 64 nodes.
    }

    template <typename T>
    T& QuadTree<T>::GetRoot()
    {
        // The root is guaranteed to exist
        return m_treeData.at(0);
    }
    
    template <typename T>
    const T& QuadTree<T>::GetRoot() const
    {
        // The root is guaranteed to exist
        return m_treeData.at(0);
    }
    
    template <typename T>
    T& QuadTree<T>::GetRootAndEnable()
    {
        // The root is guaranteed to exist
        EnableNodeIndex(0);
        return m_treeData.at(0);
    }
    
    template <typename T>
    T& QuadTree<T>::GetChild(const T& node, QuadTree<T>::ChildIndex childIndex)
    {
        return const_cast<T&>(const_cast<const QuadTree<T>*>(this)->GetChild(node, childIndex));
    }

    template <typename T>
    const T& QuadTree<T>::GetChild(const T& node, QuadTree<T>::ChildIndex childIndex) const
    {
        uint32_t index = GetChildIndex(GetIndex(node), childIndex);
        AZ_Assert(index < m_treeData.size(), "QuadTree: Cannot get the child of a leaf node.");
        return m_treeData.at(index);
    }
    
    template <typename T>
    T& QuadTree<T>::GetChildAndEnable(const T& node, ChildIndex childIndex)
    {
        uint32_t index = GetChildIndex(GetIndex(node), childIndex);
        AZ_Assert(index < m_treeData.size(), "QuadTree: Cannot get the child of a leaf node.");
        EnableNodeIndex(index);
        return m_treeData.at(index);
    }
    
    template <typename T>
    T& QuadTree<T>::GetParent(const T& node)
    {
        return const_cast<T&>(const_cast<const QuadTree<T>*>(this)->GetParent(node));
    }
    
    template <typename T>
    const T& QuadTree<T>::GetParent(const T& node) const
    {
        uint32_t index = GetIndex(node);
        uint32_t parentIndex = (index - 1) >> 2;
        return m_treeData.at(parentIndex);
    }

    template <typename T>
    typename QuadTree<T>::ChildIndex QuadTree<T>::GetEnabledChildrenIndices(const T& node) const
    {
        uint32_t dataIndex = GetIndex(node);
        uint32_t firstChildIndex = GetChildIndex(dataIndex, 0);
        uint32_t bitTypeIndex = firstChildIndex / BitsPerElement;
        uint32_t bitIndex = firstChildIndex % BitsPerElement;

        return (m_treeEnabledBits.at(bitTypeIndex) >> bitIndex) & 0xb1111;
    }
    
    template <typename T>
    uint32_t QuadTree<T>::GetDepth(const T& node) const
    {
        return GetDepthFromIndex(GetIndex(node));
    }
    
    template <typename T>
    void QuadTree<T>::EnableNode(const T& node)
    {
        EnableNodeIndex(GetIndex(node));
    }
    
    template <typename T>
    void QuadTree<T>::DisableNode(const T& node)
    {
        DisableNodeIndex(GetIndex(node));
    }
    
    template <typename T>
    bool QuadTree<T>::IsNodeEnabled(const T& node) const
    {
        return IsNodeIndexEnabled(GetIndex(node));
    }
    
    template <typename T>
    T& QuadTree<T>::GetNodeAtPosition(float x, float y, uint32_t maxDepth)
    {
        // Check if coord is in the quad tree's bounds
        AZ_Assert(x >= 0.0f && y >= 0.0f && x <= m_physicalSize && y <= m_physicalSize,
            "QuadTree: Requested node outside the bounds of the quadtree");

        // Normalize into uv space
        x *= m_rcpPhysicalSize;
        y *= m_rcpPhysicalSize;

        uint32_t index = 0;
        T& node = m_treeData.at(0); // default to root node, even if its disabled.

        while (IsNodeIndexEnabled(index))
        {
            node = m_treeData.at(index);

            // Determine which child needs to be checked next iterration
            ChildIndex childIndex = x < 0.5f ?
                (y < 0.5f ? ChildIndex::TL : ChildIndex::BL) :
                (y < 0.5f ? ChildIndex::TR : ChildIndex::BR);
            index = GetChildIndex(index, childIndex);

            // Renormalize x and y to coordinate space of child
            x = (x < 0.5f ? x : x - 0.5f) * 2.0f;
            y = (y < 0.5f ? y : y - 0.5f) * 2.0f;
        }

        return node;
    }
    
    template <typename T>
    uint32_t QuadTree<T>::GetIndex(const T& node) const
    {
        AZ_Assert(&node >= m_treeData.begin() && &node < m_treeData.end(), "QuadTree: Node is not in the bounds of QuadTree data");
        return aznumeric_cast<uint32_t>(&node - m_treeData.begin());
    }

    template <typename T>
    uint32_t QuadTree<T>::GetChildIndex(uint32_t index, ChildIndex childIndex) const
    {
        return (index << 2) + 1 + childIndex;
    }

    template <typename T>
    void QuadTree<T>::EnableNodeIndex(uint32_t index)
    {
        uint32_t bitTypeIndex = index / BitsPerElement;
        uint32_t bitIndex = index % BitsPerElement;
        m_treeEnabledBits.at(bitTypeIndex) |= 1ll << bitIndex;
    }
    
    template <typename T>
    void QuadTree<T>::DisableNodeIndex(uint32_t index)
    {
        uint32_t bitTypeIndex = index / BitsPerElement;
        uint32_t bitIndex = index % BitsPerElement;
        m_treeEnabledBits.at(bitTypeIndex) &= ~(BitHolderType)(1 << bitIndex);
    }

    template <typename T>
    bool QuadTree<T>::ChildIsEnabled(ChildIndex bitFlags, ChildIndex specificChild)
    {
        return static_cast<uint8_t>(bitFlags) & static_cast<uint8_t>(specificChild);
    }

    template <typename T>
    bool QuadTree<T>::IsNodeIndexEnabled(uint32_t index) const
    {
        uint32_t bitTypeIndex = index / BitsPerElement;
        uint32_t bitIndex = index % BitsPerElement;
        return m_treeEnabledBits.size() > bitTypeIndex && (m_treeEnabledBits.at(bitTypeIndex) & uint32_t(1 << bitIndex)) > 0;
    }

    template <typename T>
    uint32_t QuadTree<T>::NumNodesNeededForDepth(uint32_t depth) const
    {
        //! returns the total number of nodes needed to represent a full quad tree of some depth
        //! 1, 5, 21, 85, 341, etc.
        return ((4 << ((depth + 1) * 2)) - 4) / 12;
    }
    
    template <typename T>
    uint32_t QuadTree<T>::GetDepthFromIndex(uint32_t index) const
    {
        // Returns the tree depth level from an index.
        // Using az_clz_u64 to do a fast integer log2. Original function below.
        // return aznumeric_cast<uint32_t>(log2f(3.0f * index + 1.0f) / 2.0f);
        return (31 - az_clz_u32(3 * index + 1)) >> 1;
    }

}

#include <TerrainRenderer/QuadTree.inl>

#pragma optimize("", on)
