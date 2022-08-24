/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentAdapter.h>

#include <Components/CloudsComponent.h>
#include <Components/CloudsComponentConfig.h>
#include <Components/CloudsComponentController.h>

namespace Clouds
{
    static constexpr const char* const CloudsEditorComponentTypeId =
        "{0B30C72C-064B-4AF6-B08C-435EB7C1E599}";

    class CloudsEditorComponent final
        : public AzToolsFramework::Components::EditorComponentAdapter<CloudsComponentController, CloudsComponent, CloudsComponentConfig>
    {
    public:

        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<CloudsComponentController, CloudsComponent, CloudsComponentConfig>;
        AZ_EDITOR_COMPONENT(Clouds::CloudsEditorComponent, Clouds::CloudsEditorComponentTypeId, BaseClass);

        static void Reflect(AZ::ReflectContext* context);

        CloudsEditorComponent() = default;
        CloudsEditorComponent(const CloudsComponentConfig& config);

        void Activate() override;
        void Deactivate() override;

    private:

        //! EditorRenderComponentAdapter
        AZ::u32 OnConfigurationChanged() override;
    };
} // namespace Clouds
