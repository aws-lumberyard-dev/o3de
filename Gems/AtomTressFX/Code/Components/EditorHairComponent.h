/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            };
        } // namespace Hair
    } // namespace Render
} // namespace AZ
