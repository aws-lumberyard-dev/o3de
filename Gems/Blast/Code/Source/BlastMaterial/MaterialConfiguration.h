/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>

namespace Blast
{
    //! Properties of a blast material.
    struct MaterialConfiguration
    {
        AZ_TYPE_INFO(Blast::MaterialConfiguration, "{BEC875B1-26E4-4A4A-805E-0E880372720D}");

        static void Reflect(AZ::ReflectContext* context);

        float m_health = 1.0f;
        float m_forceDivider = 1.0f;
        float m_minDamageThreshold = 0.0f;
        float m_maxDamageThreshold = 1.0f;
        float m_stressLinearFactor = 1.0f;
        float m_stressAngularFactor = 1.0f;
    };
} // namespace Blast
