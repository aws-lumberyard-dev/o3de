/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>

namespace Clouds
{
    class CloudsComponentConfig final : public AZ::ComponentConfig
    {
    public:
        AZ_RTTI(Clouds::CloudsComponentConfig, "{755B7256-040E-477B-8E4E-292DB0A66CA9}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        bool m_enabled = true;
    };

} // namespace Clouds
