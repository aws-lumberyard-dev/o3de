
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace ONNXRuntime
{
    class ONNXRuntimeRequests
    {
    public:
        AZ_RTTI(ONNXRuntimeRequests, "{8896A406-AD22-428A-ACBB-ED502D91A5F0}");
        virtual ~ONNXRuntimeRequests() = default;
        // Put your public methods here
    };
    
    class ONNXRuntimeBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ONNXRuntimeRequestBus = AZ::EBus<ONNXRuntimeRequests, ONNXRuntimeBusTraits>;
    using ONNXRuntimeInterface = AZ::Interface<ONNXRuntimeRequests>;

} // namespace ONNXRuntime
