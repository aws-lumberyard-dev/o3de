/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Viewport/ViewportUiManagerTests.h>

namespace UnitTest
{

    void ViewportManagerWrapper::Create()
    {
        m_viewportManager = AZStd::make_unique<ViewportUiManagerTestable>();
        m_viewportManager->ConnectViewportUiBus(AzToolsFramework::ViewportUi::DefaultViewportId);
        m_mockRenderOverlay = AZStd::make_unique<QWidget>();
        m_parentWidget = AZStd::make_unique<QWidget>();
        m_viewportManager->InitializeViewportUi(m_parentWidget.get(), m_mockRenderOverlay.get());
    }

    void ViewportManagerWrapper::Destroy()
    {
        m_viewportManager->DisconnectViewportUiBus();
        m_viewportManager.reset();
        m_mockRenderOverlay.reset();
        m_parentWidget.reset();
    }

    TEST_F(ViewportUiManagerTestFixture, CreateClusterAddsNewClusterAndReturnsId)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);

        EXPECT_TRUE(clusterEntry != m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().end());
        EXPECT_TRUE(clusterEntry->second.get() != nullptr);
    }

    TEST_F(ViewportUiManagerTestFixture, CreateClusterButtonAddsNewButtonAndReturnsId)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);

        EXPECT_TRUE(clusterEntry->second->GetButton(buttonId) != nullptr);
    }

    TEST_F(ViewportUiManagerTestFixture, SetClusterActiveButtonSetsButtonStateToActive)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);
        auto button = clusterEntry->second->GetButton(buttonId);

        m_viewportManagerWrapper.GetViewportManager()->SetClusterActiveButton(clusterId, buttonId);

        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Selected);
    }

    TEST_F(ViewportUiManagerTestFixture, ClearClusterActiveButtonSetsButtonStateToDeselected)
    {
        // setup
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);
        auto button = clusterEntry->second->GetButton(buttonId);

        // first set a button to active
        m_viewportManagerWrapper.GetViewportManager()->SetClusterActiveButton(clusterId, buttonId);
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Selected);

        // clear the active button on the cluster
        m_viewportManagerWrapper.GetViewportManager()->ClearClusterActiveButton(clusterId);
        // the button should now be deselected
        EXPECT_TRUE(button->m_state == AzToolsFramework::ViewportUi::Internal::Button::State::Deselected);
    }

    TEST_F(ViewportUiManagerTestFixture, RegisterClusterEventHandlerConnectsHandlerToClusterEvent)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        auto buttonId = m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");

        // create a handler which will be triggered by the cluster
        bool handlerTriggered = false;
        auto testButtonId = ButtonId(buttonId);
        AZ::Event<ButtonId>::Handler handler(
            [&handlerTriggered, testButtonId](ButtonId buttonId)
            {
                if (buttonId == testButtonId)
                {
                    handlerTriggered = true;
                }
            });

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);

        // trigger the cluster
        m_viewportManagerWrapper.GetViewportManager()->RegisterClusterEventHandler(clusterId, handler);
        clusterEntry->second->PressButton(buttonId);

        EXPECT_TRUE(handlerTriggered);
    }

    TEST_F(ViewportUiManagerTestFixture, RemoveClusterRemovesClusterFromViewportUi)
    {
        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        m_viewportManagerWrapper.GetViewportManager()->RemoveCluster(clusterId);

        auto clusterEntry = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId);

        EXPECT_TRUE(clusterEntry == m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().end());
    }

    TEST_F(ViewportUiManagerTestFixture, SetClusterVisibleChangesClusterVisibility)
    {
        m_viewportManagerWrapper.GetMockRenderOverlay()->setVisible(true);

        auto clusterId = m_viewportManagerWrapper.GetViewportManager()->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
        m_viewportManagerWrapper.GetViewportManager()->CreateClusterButton(clusterId, "");
        m_viewportManagerWrapper.GetViewportManager()->Update();

        m_viewportManagerWrapper.GetViewportManager()->SetClusterVisible(clusterId, false);
        auto cluster = m_viewportManagerWrapper.GetViewportManager()->GetClusterMap().find(clusterId)->second;

        bool visible =
            m_viewportManagerWrapper.GetViewportManager()->GetViewportUiDisplay()->IsViewportUiElementVisible(cluster->GetViewportUiElementId());
        EXPECT_FALSE(visible);

        m_viewportManagerWrapper.GetViewportManager()->SetClusterVisible(clusterId, true);
        visible =
            m_viewportManagerWrapper.GetViewportManager()->GetViewportUiDisplay()->IsViewportUiElementVisible(cluster->GetViewportUiElementId());
        EXPECT_TRUE(visible);
    }
} // namespace UnitTest
