/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabDependencyViewerEditorSystemComponent.h>
#include <PrefabDependencyTree.h>

#include <DirectedGraph.h>
#include <PrefabDependencyViewerFixture.h>

#include <AzCore/std/algorithm.h>

namespace PrefabDependencyViewer
{
    using DirectedGraph = Utils::DirectedTree;
    using TestComponent = PrefabDependencyViewerEditorSystemComponent;
    using NodeList      = AZStd::vector<Utils::Node*>;

    void EXPECT_STR_EQ(const char* expected, AZStd::string_view result)
    {
        EXPECT_STREQ(expected, result.data());
    }

    TEST_F(PrefabDependencyViewerFixture, INVALID_TEMPLATE_ID)
    {
        TreeOutcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(
                            InvalidTemplateId, m_prefabSystemComponent);

        EXPECT_FALSE(outcome.IsSuccess());
    }
    
    TEST_F(PrefabDependencyViewerFixture, EMPTY_PREFAB_NO_SOURCE_TEST)
    {
        TemplateId tid = 10;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSON"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(10, m_prefabSystemComponent);
        EXPECT_FALSE(outcome.IsSuccess());
    }
    
    TEST_F(PrefabDependencyViewerFixture, EMPTY_PREFAB_WITH_SOURCE_TEST)
    {
        TemplateId tid = 2000;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSONWithSource"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_TRUE(outcome.IsSuccess());

        PrefabDependencyTree tree = outcome.GetValue();
        EXPECT_STR_EQ("Prefabs/emptyJSONWithSource.prefab", tree.GetRoot()->GetMetaData()->GetDisplayName());

        EXPECT_TRUE(tree.GetRoot()->GetChildren().empty());
    }

    
    TEST_F(PrefabDependencyViewerFixture, NESTED_PREFAB_WITH_ATLEAST_ONE_INVALID_SOURCE_FILE)
    {
        TemplateId tid = 52893;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["NestedPrefabWithAtleastOneInvalidNestedInstance"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_FALSE(outcome.IsSuccess());
    }

    TEST_F(PrefabDependencyViewerFixture, VALID_NESTED_PREFAB)
    {
        TemplateId tid = 2022412;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["ValidPrefab"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_TRUE(outcome.IsSuccess());

        PrefabDependencyTree tree = outcome.GetValue();

        EXPECT_EQ(nullptr, tree.GetRoot()->GetParent());
        EXPECT_EQ(3, tree.GetRoot()->GetChildren().size());

        // Check Level 1 Nodes
        Utils::ChildrenList level1Nodes = tree.GetRoot()->GetChildren();
        Utils::ChildrenList level11Nodes = FindNodes(level1Nodes, "Prefabs/level11.prefab");
        EXPECT_EQ(1, level11Nodes.size());

        Utils::ChildrenList level12Nodes = FindNodes(level1Nodes, "Prefabs/level12.prefab");
        EXPECT_EQ(1, level12Nodes.size());

        Utils::ChildrenList level13Nodes = FindNodes(level1Nodes, "Prefabs/level13.prefab");
        EXPECT_EQ(1, level13Nodes.size());

        EXPECT_TRUE(FindNodes(level1Nodes, "asa.prefab").empty());

        Utils::NodePtr level11Node = level11Nodes[0];
        Utils::NodePtr level12Node = level12Nodes[0];
        Utils::NodePtr level13Node = level13Nodes[0];

        EXPECT_EQ(tree.GetRoot().get(), level11Node->GetParent());
        EXPECT_EQ(tree.GetRoot().get(), level12Node->GetParent());
        EXPECT_EQ(tree.GetRoot().get(), level13Node->GetParent());

        EXPECT_EQ(1, level11Node->GetChildren().size());
        EXPECT_TRUE(level12Node->GetChildren().empty());
        EXPECT_EQ(2, level13Node->GetChildren().size());

        // Check Level 2 Nodes

        Utils::ChildrenList level13Children = level13Node->GetChildren();
        auto it = level13Children.begin();

        Utils::NodePtr level21Node = *(level11Node->GetChildren().begin());
        Utils::NodePtr level22Node = *it;
        ++it;
        Utils::NodePtr level23Node = *it;

        EXPECT_EQ(level11Node.get(), level21Node->GetParent());
        EXPECT_STR_EQ("Prefabs/level12.prefab", level21Node->GetMetaData()->GetDisplayName());

        EXPECT_EQ(level13Node.get(), level22Node->GetParent());
        EXPECT_STR_EQ("Prefabs/level22.prefab", level22Node->GetMetaData()->GetDisplayName());

        EXPECT_EQ(level13Node.get(), level23Node->GetParent());
        EXPECT_STR_EQ("Prefabs/level23.prefab", level23Node->GetMetaData()->GetDisplayName());

        EXPECT_TRUE(level21Node->GetChildren().empty());
        EXPECT_TRUE(level22Node->GetChildren().empty());
        EXPECT_EQ(1, level23Node->GetChildren().size());

        Utils::NodePtr level31Node = *(level23Node->GetChildren().begin());
        EXPECT_EQ(level23Node.get(), level31Node->GetParent());

        EXPECT_STR_EQ("Prefabs/level31.prefab", level31Node->GetMetaData()->GetDisplayName());

        EXPECT_TRUE(level31Node->GetChildren().empty());
    }
} // namespace PrefabDependencyViewer
