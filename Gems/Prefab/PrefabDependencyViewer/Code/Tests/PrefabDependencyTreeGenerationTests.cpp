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
        EXPECT_EQ(strlen(expected), result.size());
        EXPECT_STREQ(expected, result.data());
    }

    TEST_F(PrefabDependencyViewerFixture, INVALID_TEMPLATE_ID)
    {
        TreeOutcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(
                            InvalidTemplateId, m_prefabSystemComponent);

        EXPECT_EQ(false, outcome.IsSuccess());
    }

    TEST_F(PrefabDependencyViewerFixture, EMPTY_PREFAB_NO_SOURCE_TEST)
    {
        TemplateId tid = 10;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSON"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(10, m_prefabSystemComponent);
        EXPECT_EQ(false, outcome.IsSuccess());
    }

    TEST_F(PrefabDependencyViewerFixture, EMPTY_PREFAB_WITH_SOURCE_TEST)
    {
        TemplateId tid = 2000;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["emptyJSONWithSource"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_EQ(true, outcome.IsSuccess());

        PrefabDependencyTree tree = outcome.GetValue();
        EXPECT_STR_EQ("Prefabs/emptySavedJSON.prefab", tree.GetRoot()->GetMetaData()->GetDisplayName());

        EXPECT_EQ(0, tree.GetRoot()->GetChildren().size());
    }

    TEST_F(PrefabDependencyViewerFixture, NESTED_PREFAB_WITH_ATLEAST_ONE_INVALID_SOURCE_FILE)
    {
        TemplateId tid = 52893;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["NestedPrefabWithAtleastOneInvalidNestedInstance"]));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/goodPrefab.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(5));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("")))
            .Times(1)
            .WillRepeatedly(::testing::Return(InvalidTemplateId));

        // Depending on which TemplateId stack gets to first
        // you can or can't call FindTemplateDom for GoodNestedPrefab's
        // TemplateId
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(5))
            .Times(0);

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_EQ(false, outcome.IsSuccess());
    }

    TEST_F(PrefabDependencyViewerFixture, VALID_NESTED_PREFAB)
    {
        TemplateId tid = 2022412;
        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(tid))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["ValidPrefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level11.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(10000));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level12.prefab")))
            .Times(2)
            .WillRepeatedly(::testing::Return(121));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level13.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(12141));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(10000))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level11Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(121))
            .Times(2)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level12Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(12141))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level13Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level22.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(240121));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level23.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(123));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(240121))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level22Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(123))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level23Prefab"]));

        EXPECT_CALL(*m_prefabSystemComponent, GetTemplateIdFromFilePath(AZ::IO::PathView("Prefabs/level31.prefab")))
            .Times(1)
            .WillRepeatedly(::testing::Return(221));

        EXPECT_CALL(*m_prefabSystemComponent, FindTemplateDom(221))
            .Times(1)
            .WillRepeatedly(::testing::ReturnRef(m_prefabDomsCases["level31Prefab"]));

        Outcome outcome = PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponent);
        EXPECT_EQ(true, outcome.IsSuccess());

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

        EXPECT_EQ(0, FindNodes(level1Nodes, "asa.prefab").size());

        Utils::NodePtr level11Node = level11Nodes[0];
        Utils::NodePtr level12Node = level12Nodes[0];
        Utils::NodePtr level13Node = level13Nodes[0];

        EXPECT_EQ(tree.GetRoot().get(), level11Node->GetParent());
        EXPECT_EQ(tree.GetRoot().get(), level12Node->GetParent());
        EXPECT_EQ(tree.GetRoot().get(), level13Node->GetParent());

        EXPECT_EQ(1, level11Node->GetChildren().size());
        EXPECT_EQ(0, level12Node->GetChildren().size());
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

        EXPECT_EQ(0, level21Node->GetChildren().size());
        EXPECT_EQ(0, level22Node->GetChildren().size());
        EXPECT_EQ(1, level23Node->GetChildren().size());

        Utils::NodePtr level31Node = *(level23Node->GetChildren().begin());
        EXPECT_EQ(level23Node.get(), level31Node->GetParent());

        EXPECT_STR_EQ("Prefabs/level31.prefab", level31Node->GetMetaData()->GetDisplayName());

        EXPECT_EQ(0, level31Node->GetChildren().size());
    }

} // namespace PrefabDependencyViewer
