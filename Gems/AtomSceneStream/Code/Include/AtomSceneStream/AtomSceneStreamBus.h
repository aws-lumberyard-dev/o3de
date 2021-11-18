
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace AtomSceneStream
    {
        class AtomSceneStreamRequests
        {
        public:
            AZ_RTTI(AtomSceneStreamRequests, "{687b8326-bc16-4143-8aad-c178fca6aefa}");
            virtual ~AtomSceneStreamRequests() = default;
            // Put your public methods here
        };

        class AtomSceneStreamBusTraits
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            //////////////////////////////////////////////////////////////////////////
        };

        using AtomSceneStreamRequestBus = AZ::EBus<AtomSceneStreamRequests, AtomSceneStreamBusTraits>;
        using AtomSceneStreamInterface = AZ::Interface<AtomSceneStreamRequests>;

    } // namespace AtomSceneStream
}

