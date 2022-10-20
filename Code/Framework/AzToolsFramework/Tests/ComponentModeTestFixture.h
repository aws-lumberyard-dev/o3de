/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzToolsFramework/ViewportUi/ViewportUiManager.h>

#include <Tests/Prefab/PrefabTestFixture.h>
#include <Tests/Viewport/ViewportUiManagerTests.h>

namespace UnitTest
{
    class ComponentModeTestFixture
        : public ToolsApplicationFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;

        ViewportManagerWrapper m_viewportManagerWrapper;
    };

    class ComponentModeSwitcherTestFixture : public PrefabTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;
        void TearDownEditorFixtureImpl() override;

        ViewportManagerWrapper m_viewportManagerWrapper;
    };
} // namespace UnitTest
