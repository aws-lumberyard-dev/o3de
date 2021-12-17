
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace LBC
{
    class LBCRequests
    {
    public:
        AZ_RTTI(LBCRequests, "{2e55efcf-dad2-4dac-bc20-7d3b4167a73b}");
        virtual ~LBCRequests() = default;
        // Put your public methods here

        virtual void SetRedBallEntityId(AZ::EntityId entityId) = 0;
        virtual AZ::EntityId GetRedBallEntityId() const = 0;

        virtual void SetBlueBallEntityId(AZ::EntityId entityId) = 0;
        virtual AZ::EntityId GetBlueBallEntityId() const = 0;
    };
    
    class LBCBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using LBCRequestBus = AZ::EBus<LBCRequests, LBCBusTraits>;
    using LBCInterface = AZ::Interface<LBCRequests>;

} // namespace LBC
