/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/EBus/EBus.h>

#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    class PaintBrushComponentRequests : public AZ::EBusTraits
    {
    public:
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityComponentIdPair;
        using MutexType = AZStd::recursive_mutex;

        virtual void SavePaintLayer() = 0;

    protected:
        ~PaintBrushComponentRequests() = default;
    };

    using PaintBrushComponentRequestBus = AZ::EBus<PaintBrushComponentRequests>;
} // namespace AzToolsFramework
