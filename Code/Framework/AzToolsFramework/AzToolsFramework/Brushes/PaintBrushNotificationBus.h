/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    class PaintBrushNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual void OnIntensityChanged([[maybe_unused]] const float radius) { }
        virtual void OnOpacityChanged([[maybe_unused]] const float radius) { }
        virtual void OnRadiusChanged([[maybe_unused]] const float radius) { }
    };

    using PaintBrushNotificationBus = AZ::EBus<PaintBrushNotifications>;
} // namespace AzToolsFramework
