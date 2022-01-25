/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/UnitTest/TestTypes.h>
#include <gmock/gmock.h>

#include <TerrainRenderer/QuadTree.h>

#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/vector.h>

namespace UnitTest
{
    class QuadTreeTests
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();
        }

        void TearDown() override
        {
            UnitTest::AllocatorsTestFixture::TearDown();
        }
    };
    
    TEST_F(QuadTreeTests, Construction)
    {
        Terrain::QuadTree<uint32_t> m_quadTree(1024.0f);
    }

    TEST_F(QuadTreeTests, GetRoot)
    {
        Terrain::QuadTree<uint32_t> m_quadTree(1024.0f);
        m_quadTree.GetRoot() = 100;
        EXPECT_EQ(m_quadTree.GetRoot(), 100);
        EXPECT_FALSE(m_quadTree.IsNodeEnabled(m_quadTree.GetRoot()));
        m_quadTree.EnableNode(m_quadTree.GetRoot());
        EXPECT_TRUE(m_quadTree.IsNodeEnabled(m_quadTree.GetRoot()));
        

        AZStd::vector<int> foo;
        AZStd::fill(foo.begin(), foo.end(), 10);
        AZStd::span<const int> fooSpan(foo.begin(), foo.end());
    }

    /*
        void ResizeDepth(uint32_t depth);

        T& GetRoot();
        T& GetRootAndEnable();
        T& GetChild(const T& node, ChildIndex childIndex);
        T& GetChildAndEnable(const T& node, ChildIndex childIndex);
        T& GetParent(const T& node);

        ChildIndex GetEnabledChildrenIndices(const T& node);
        uint32_t GetChildDepth(const T& node);
        bool IsNull(const T& node);
        void DisableNode(const T& node);

        T& GetNodeAtPosition(float x, float y, uint32_t maxDepth = AZStd::numeric_limits<uint32_t>::max());

        static bool ChildIsEnabled(ChildIndex bitFlags, ChildIndex specificChild);

    */
}
