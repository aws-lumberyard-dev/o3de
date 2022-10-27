/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <GradientSignal/Components/RidgedGradientComponent.h>

namespace GradientSignal
{
    class EditorRidgedGradientComponent
        : public EditorGradientComponentBase<RidgedGradientComponent, RidgedGradientConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<RidgedGradientComponent, RidgedGradientConfig>;
        AZ_EDITOR_COMPONENT(EditorRidgedGradientComponent, EditorRidgedGradientComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Ridged Gradient Modifier";
        static constexpr const char* const s_componentDescription = "'Ridges' a gradient's values by shifting the midpoint and taking the absolute value";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.svg";
        static constexpr const char* const s_helpUrl = "";
    };
}
