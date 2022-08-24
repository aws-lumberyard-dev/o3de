/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <Components/CloudsComponentConfig.h>
#include <Components/CloudsComponentController.h>

namespace Clouds
{
    static constexpr const char* const CloudsComponentTypeId = "{C45164D7-2C0C-4181-88A4-5A519B5BDC0F}";

    class CloudsComponent final
        : public AzFramework::Components::ComponentAdapter<CloudsComponentController, CloudsComponentConfig>
    {
    public:
        using BaseClass = AzFramework::Components::ComponentAdapter<CloudsComponentController, CloudsComponentConfig>;
        AZ_COMPONENT(Clouds::CloudsComponent, Clouds::CloudsComponentTypeId, BaseClass);

        CloudsComponent() = default;
        CloudsComponent(const CloudsComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);

        void Activate() override;
        void Deactivate() override;
    };
} // namespace Clouds
