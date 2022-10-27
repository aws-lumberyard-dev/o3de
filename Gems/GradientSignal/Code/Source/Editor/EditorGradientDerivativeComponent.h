/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <GradientSignal/Components/GradientDerivativeComponent.h>

namespace GradientSignal
{
    class EditorGradientDerivativeComponent
        : public EditorGradientComponentBase<GradientDerivativeComponent, GradientDerivativeConfig>
    {
    public:
        using BaseClassType = EditorGradientComponentBase<GradientDerivativeComponent, GradientDerivativeConfig>;
        AZ_EDITOR_COMPONENT(EditorGradientDerivativeComponent, EditorGradientDerivativeComponentTypeId, BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Gradient Modifiers";
        static constexpr const char* const s_componentName = "Gradient Derivative Modifier";
        static constexpr const char* const s_componentDescription = "Calculates the amount of change between adjacent gradient values";
        static constexpr const char* const s_icon = "Editor/Icons/Components/GradientModifier.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/GradientModifier.svg";
        static constexpr const char* const s_helpUrl = "";
    };
}
