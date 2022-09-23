/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <Components/MacroColorFalloffGradientComponent.h>

namespace GradientSignal
{
    inline constexpr AZ::TypeId EditorMacroColorFalloffGradientComponentTypeId{ "{61DB81E0-B898-4995-8ABA-2E2C9F70E618}" };

    class EditorMacroColorFalloffGradientComponent
        : public EditorGradientComponentBase<MacroColorFalloffGradientComponent, MacroColorFalloffGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<MacroColorFalloffGradientComponent, MacroColorFalloffGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorMacroColorFalloffGradientComponent, EditorMacroColorFalloffGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Macro Color Falloff Gradient";
        static constexpr const char* const s_componentDescription = "Generates a gradient based on distance from a color in the macro color";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Gradient.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Gradient.svg";
        static constexpr const char* const s_helpUrl = "";
    };
}
