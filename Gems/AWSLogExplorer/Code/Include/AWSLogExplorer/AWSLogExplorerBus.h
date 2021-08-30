/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace AWSLogExplorer
{
    class AWSLogExplorerRequests
    {
    public:
        AZ_RTTI(AWSLogExplorerRequests, "{a0d15421-9456-4b76-bea1-87369d3ccc09}");
        virtual ~AWSLogExplorerRequests() = default;
        // Put your public methods here
    };
    
    class AWSLogExplorerBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using AWSLogExplorerRequestBus = AZ::EBus<AWSLogExplorerRequests, AWSLogExplorerBusTraits>;
    using AWSLogExplorerInterface = AZ::Interface<AWSLogExplorerRequests>;

} // namespace AWSLogExplorer
