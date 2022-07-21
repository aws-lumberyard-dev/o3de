
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace Ort {
    struct Env;
    struct AllocatorWithDefaultOptions;
}

namespace ONNX
{
    class ONNXRequests
    {
    public:
        AZ_RTTI(ONNXRequests, "{F8599C7E-CDC7-4A72-A296-2C043D1E525A}");
        virtual ~ONNXRequests() = default;
        // Put your public methods here
        virtual Ort::Env* GetEnv() = 0;
        virtual Ort::AllocatorWithDefaultOptions* GetAllocator() = 0;
    };
    
    class ONNXBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using ONNXRequestBus = AZ::EBus<ONNXRequests, ONNXBusTraits>;
    using ONNXInterface = AZ::Interface<ONNXRequests>;

} // namespace ONNX
