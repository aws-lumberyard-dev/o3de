/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TerrainRenderer/Components/TerrainGradientMacroMaterialComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainGradientMacroMaterialComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainGradientMacroMaterialComponent, TerrainGradientMacroMaterialConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainGradientMacroMaterialComponent, TerrainGradientMacroMaterialConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainGradientMacroMaterialComponent, "{CF3690D7-FD15-4EA9-AAF1-01273017578C}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain Gradient Macro Material";
        static constexpr const char* const s_componentDescription = "Generates a terrain macro material from a series of gradients";
        static constexpr const char* const s_icon = "Editor/Icons/Components/TerrainMacroMaterial.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainMacroMaterial.svg";
        static constexpr const char* const s_helpUrl = "";
    };
}
