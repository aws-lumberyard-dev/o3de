
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Ocean
{
    class OceanRequests
    {
    public:
        AZ_RTTI(OceanRequests, "{b83a9832-edb2-4365-8ff8-815d9ce67736}");
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
