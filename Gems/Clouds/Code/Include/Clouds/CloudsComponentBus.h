
#pragma once

#include <AzCore/Component/Component.h>

namespace Clouds
{
    class CloudsComponentRequests
        : public AZ::ComponentBus
    {
    public:
        AZ_RTTI(CloudsComponentRequests, "{8D84450E-7CF3-49B6-9387-5F41D4A136CE}");
        virtual ~CloudsComponentRequests() = default;

        // Put your public methods here
    };

    using CloudsComponentRequestBus = AZ::EBus<CloudsComponentRequests>;

} // namespace Clouds
