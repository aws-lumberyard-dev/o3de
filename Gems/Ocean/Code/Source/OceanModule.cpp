

#include <OceanModuleInterface.h>
#include <OceanSystemComponent.h>

namespace Ocean
{
    class OceanModule
        : public OceanModuleInterface
    {
    public:
        AZ_RTTI(OceanModule, "{674ba7f5-0a90-49fb-adba-d918c3b24625}", OceanModuleInterface);
        AZ_CLASS_ALLOCATOR(OceanModule, AZ::SystemAllocator, 0);
    };
}// namespace Ocean

AZ_DECLARE_MODULE_CLASS(Gem_Ocean, Ocean::OceanModule)
