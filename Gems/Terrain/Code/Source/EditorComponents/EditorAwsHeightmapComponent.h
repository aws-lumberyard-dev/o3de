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

#include <Components/AwsHeightmapComponent.h>
#include <GradientSignal/Editor/EditorGradientComponentBase.h>
#include <LmbrCentral/Component/EditorWrappedComponentBase.h>


namespace Terrain
{
    class EditorAwsHeightmapComponent
        : public GradientSignal::EditorGradientComponentBase<AwsHeightmapComponent, AwsHeightmapConfig>
    {
    public:
        using BaseClassType = GradientSignal::EditorGradientComponentBase<AwsHeightmapComponent, AwsHeightmapConfig>;
        AZ_EDITOR_COMPONENT(EditorAwsHeightmapComponent, "{F16AE095-C6AF-472F-9764-1098D4B3749C}", BaseClassType);
        static void Reflect(AZ::ReflectContext* context);

        static constexpr const char* const s_categoryName = "Terrain";
        static constexpr const char* const s_componentName = "Terrain AWS Height Provider";
        static constexpr const char* const s_componentDescription = "Provides real-world height data for a region downloaded from AWS to the terrain system";
        static constexpr const char* const s_icon = "Editor/Icons/Components/Box_Shape.svg";
        static constexpr const char* const s_viewportIcon = "Editor/Icons/Components/Viewport/Box_Shape.png";
        static constexpr const char* const s_helpUrl = "";
    };
}
