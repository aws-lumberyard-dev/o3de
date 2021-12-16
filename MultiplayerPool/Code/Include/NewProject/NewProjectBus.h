
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace NewProject
{
    class NewProjectRequests
    {
    public:
        AZ_RTTI(NewProjectRequests, "{cba8bd35-648b-4b1e-a179-22532a135178}");
        virtual ~NewProjectRequests() = default;
        // Put your public methods here
    };
    
    class NewProjectBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using NewProjectRequestBus = AZ::EBus<NewProjectRequests, NewProjectBusTraits>;
    using NewProjectInterface = AZ::Interface<NewProjectRequests>;

} // namespace NewProject
