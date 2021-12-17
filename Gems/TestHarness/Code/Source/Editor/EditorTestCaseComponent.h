/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include "Components/TestCaseComponent.h"
#include <AzCore/Module/Module.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityBus.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace TestHarness
{
    class EditorTestCaseComponent
        : public LmbrCentral::EditorWrappedComponentBase<TestCaseComponent, TestCaseConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TestCaseComponent, TestCaseConfig>;
        AZ_EDITOR_COMPONENT(EditorTestCaseComponent, "{1E1C575B-2B23-42F2-8B74-74871BB39356}", BaseClassType);

        static void Reflect(AZ::ReflectContext* context);
        static constexpr const char* const s_categoryName = "Test";
        static constexpr const char* const s_componentName = "TestHarness Test Case";
        static constexpr const char* const s_componentDescription = "Collects and runs a group of in-game C++ and Lua tests automatically";
        static constexpr const char* const s_icon = "Editor/Icons/Components/ColliderSphere.png";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/ColliderSphere.png";
        static constexpr const char* const s_helpUrl = "https://o3de.org/docs/user-guide/components/reference/";
    };
} // namespace TestHarness
