
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Clouds
{
    class CloudsRequests
    {
    public:
        AZ_RTTI(CloudsRequests, "{50FC015D-C354-40A5-A954-48087CC93557}");
        virtual ~CloudsRequests() = default;
        // Put your public methods here
    };
    
    class CloudsBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using CloudsRequestBus = AZ::EBus<CloudsRequests, CloudsBusTraits>;
    using CloudsInterface = AZ::Interface<CloudsRequests>;

} // namespace Clouds
