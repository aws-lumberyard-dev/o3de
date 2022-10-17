/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Transform.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>

namespace UnitTest
{
    using ViewportUiDisplay = AzToolsFramework::ViewportUi::Internal::ViewportUiDisplay;
    using ViewportUiElementId = AzToolsFramework::ViewportUi::ViewportUiElementId;
    using ButtonGroup = AzToolsFramework::ViewportUi::Internal::ButtonGroup;
    using ButtonId = AzToolsFramework::ViewportUi::ButtonId;

    // child class of ViewportUiManager which exposes the protected button group and viewport display
    class ViewportUiManagerTestable : public AzToolsFramework::ViewportUi::ViewportUiManager
    {
    public:
        ViewportUiManagerTestable() = default;
        ~ViewportUiManagerTestable() override = default;

        const AZStd::unordered_map<AzToolsFramework::ViewportUi::ClusterId, AZStd::shared_ptr<ButtonGroup>>& GetClusterMap()
        {
            return m_clusterButtonGroups;
        }

        ViewportUiDisplay* GetViewportUiDisplay()
        {
            return m_viewportUi.get();
        }
    };

    class ViewportManagerWrapper
    {
    public:
        void Create();
        void Destroy();

        ViewportUiManagerTestable* GetViewportManager()
        {
            return m_viewportManager.get();
        }

        QWidget* GetMockRenderOverlay()
        {
            return m_mockRenderOverlay.get();
        }

    private:
        AZStd::unique_ptr<ViewportUiManagerTestable> m_viewportManager;
        AZStd::unique_ptr<QWidget> m_parentWidget;
        AZStd::unique_ptr<QWidget> m_mockRenderOverlay;
    };

    // sets up a parent widget and render overlay to attach the Viewport UI to
    // as well as a cluster with one button
    class ViewportUiManagerTestFixture : public ::testing::Test
    {
    public:
        ViewportUiManagerTestFixture() = default;

        ViewportManagerWrapper m_viewportManagerWrapper;

        void SetUp() override
        {
            m_viewportManagerWrapper.Create();
        }

        void TearDown() override
        {
            m_viewportManagerWrapper.Destroy();
        }
    };
} // namespace UnitTest
