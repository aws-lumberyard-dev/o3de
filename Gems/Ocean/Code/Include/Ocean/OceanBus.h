// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Ocean
{
    class OceanRequests
    {
    public:
        AZ_RTTI(OceanRequests, "{6a9fcb74-ed3a-4156-98e7-f4c630d496ea}");
        virtual ~OceanRequests() = default;
        // Put your public methods here
    };
    
    class OceanBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using OceanRequestBus = AZ::EBus<OceanRequests, OceanBusTraits>;
    using OceanInterface = AZ::Interface<OceanRequests>;

} // namespace Ocean
