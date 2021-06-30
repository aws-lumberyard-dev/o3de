/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    class PaintBrushRequests : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityComponentIdPair;
        using MutexType = AZStd::recursive_mutex;

        // PaintBrushRequests interface...
        virtual float GetRadius() const = 0;
        virtual float GetIntensity() const = 0;
        virtual float GetOpacity() const = 0;

        virtual void SetRadius(float radius) = 0;
        virtual void SetIntensity(float intensity) = 0;
        virtual void SetOpacity(float opacity) = 0;

    protected:
        ~PaintBrushRequests() = default;
    };

    using PaintBrushRequestBus = AZ::EBus<PaintBrushRequests>;
} // namespace AzToolsFramework
