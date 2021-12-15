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
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Aabb.h>

namespace Terrain
{
    /**
    * Request terrain macro material data.
    */
    class CoordinateMapperRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////
        // EBusTraits
        // singleton pattern
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::recursive_mutex;
        ////////////////////////////////////////////////////////////////////////

        virtual ~CoordinateMapperRequests() = default;

        virtual void ConvertWorldAabbToLatLong(const AZ::Aabb& worldAabb,
            float& topLatitude, float& leftLongitude, float& bottomLatitude, float& rightLongitude) = 0;
    };

    using CoordinateMapperRequestBus = AZ::EBus<CoordinateMapperRequests>;
}
