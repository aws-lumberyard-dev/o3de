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

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <Components/HairComponent.h>
#include <Components/HairComponentConfig.h>
#include <Components/HairComponentController.h>


namespace AZ
{
    namespace Render
    {
        namespace Hair
        {
            //! Visual editor representation of the hair that can be created for an entity
            //! that have an Actor component.
            //! The config data itself is held by the 'HairCOmponentConfig' that reflects the 'TressFXSettings'
            //! and by the 'HairGlobalSettings' that mainly controls the shader options.
            //! The hair data is held by the 'HairRenderObject' and the connection between the component
            //! and the handling of the data is done by the 'HairComponentController'.
            static constexpr const char* const EditorHairComponentTypeId =
                "{822A8253-4662-41B1-8623-7B2D047A4D68}";

            class EditorHairComponent final
                : public AzToolsFramework::Components::EditorComponentAdapter<HairComponentController, HairComponent, HairComponentConfig>
                , private AzFramework::EntityDebugDisplayEventBus::Handler
            {
            public:

                using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<HairComponentController, HairComponent, HairComponentConfig>;
                AZ_EDITOR_COMPONENT(AZ::Render::Hair::EditorHairComponent, Hair::EditorHairComponentTypeId, BaseClass);

                static void Reflect(AZ::ReflectContext* context);

                EditorHairComponent() = default;
                EditorHairComponent(const HairComponentConfig& config);

                void Activate() override;
                void Deactivate() override;

                // AzFramework::DebugDisplayRequestBus::Handler interface
                void DisplayEntityViewport(
                    const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;
            private:

                //! EditorRenderComponentAdapter overrides...
                AZ::u32 OnConfigurationChanged() override;

                Data::AssetId m_prevHairAssetId; // Previous loaded hair asset id.
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ
