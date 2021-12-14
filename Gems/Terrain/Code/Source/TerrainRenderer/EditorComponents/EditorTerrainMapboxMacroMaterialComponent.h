/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TerrainRenderer/Components/TerrainMapboxMacroMaterialComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>

namespace Terrain
{
    class EditorTerrainMapboxMacroMaterialComponent
        : public LmbrCentral::EditorWrappedComponentBase<TerrainMapboxMacroMaterialComponent, TerrainMapboxMacroMaterialConfig>
    {
    public:
        using BaseClassType = LmbrCentral::EditorWrappedComponentBase<TerrainMapboxMacroMaterialComponent, TerrainMapboxMacroMaterialConfig>;
        AZ_EDITOR_COMPONENT(EditorTerrainMapboxMacroMaterialComponent, "{7A34EC28-C57C-4002-9660-18A1BBD6BD88}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain Mapbox Macro Material";
        static constexpr const char* const s_componentDescription = "Provides Mapbox satellite for a region as a macro material to the terrain renderer";
        static constexpr const char* const s_icon = "Editor/Icons/Components/TerrainMacroMaterial.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/TerrainMacroMaterial.svg";
        static constexpr const char* const s_helpUrl = "";
    };
}
