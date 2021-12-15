/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorComponents/EditorCoordinateMapperComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace Terrain
{
    void EditorCoordinateMapperComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorCoordinateMapperComponent, BaseClassType>()
                ->Version(0)
            ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorCoordinateMapperComponent>(
                    "Coordinate Mapper", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Terrain")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/TerrainWorld.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/TerrainWorld.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("Level") }))
                ;
            }
        }
    }


    void EditorCoordinateMapperComponent::Init()
    {
        BaseClassType::Init();
    }

    void EditorCoordinateMapperComponent::Activate()
    {
        BaseClassType::Activate();
    }

    AZ::u32 EditorCoordinateMapperComponent::ConfigurationChanged()
    {
        return BaseClassType::ConfigurationChanged();
    }
}
