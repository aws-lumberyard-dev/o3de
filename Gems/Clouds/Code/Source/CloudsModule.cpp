

#include <CloudsModuleInterface.h>
#include <CloudsSystemComponent.h>

namespace Clouds
{
    class CloudsModule
        : public CloudsModuleInterface
    {
    public:
        AZ_RTTI(CloudsModule, "{967C196F-C37C-4C80-9D42-F3A507BD8BD7}", CloudsModuleInterface);
        AZ_CLASS_ALLOCATOR(CloudsModule, AZ::SystemAllocator, 0);
    };
}// namespace Clouds

AZ_DECLARE_MODULE_CLASS(Gem_Clouds, Clouds::CloudsModule)
