/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeTestFixture.h"
#include "ComponentModeTestDoubles.h"

#include <AzCore/UserSettings/UserSettingsComponent.h>

namespace UnitTest
{
    static void RegisterComponentModeTestComponents(AzToolsFramework::ToolsApplication* toolsApplication)
    {
        namespace AztfCmf = AzToolsFramework::ComponentModeFramework;

        toolsApplication->RegisterComponentDescriptor(AztfCmf::PlaceholderEditorComponent::CreateDescriptor());
        toolsApplication->RegisterComponentDescriptor(AztfCmf::AnotherPlaceholderEditorComponent::CreateDescriptor());
        toolsApplication->RegisterComponentDescriptor(AztfCmf::DependentPlaceholderEditorComponent::CreateDescriptor());
        toolsApplication->RegisterComponentDescriptor(
            AztfCmf::TestComponentModeComponent<AztfCmf::OverrideMouseInteractionComponentMode>::CreateDescriptor());
        toolsApplication->RegisterComponentDescriptor(AztfCmf::IncompatiblePlaceholderEditorComponent::CreateDescriptor());
    }

    void ComponentModeTestFixture::SetUpEditorFixtureImpl()
    {
        RegisterComponentModeTestComponents(GetApplication());
    }

    void ComponentModeSwitcherTestFixture::SetUpEditorFixtureImpl()
    {
        PrefabTestFixture::SetUpEditorFixtureImpl();

        RegisterComponentModeTestComponents(GetApplication());
        m_viewportManagerWrapper.Create();
    }

    void ComponentModeSwitcherTestFixture::TearDownEditorFixtureImpl()
    {
        m_viewportManagerWrapper.Destroy();

        PrefabTestFixture::TearDownEditorFixtureImpl();
    }
} // namespace UnitTest
