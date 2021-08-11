

#include <AWSLogExplorerModuleInterface.h>
#include <AWSLogExplorerSystemComponent.h>

namespace AWSLogExplorer
{
    class AWSLogExplorerModule
        : public AWSLogExplorerModuleInterface
    {
    public:
        AZ_RTTI(AWSLogExplorerModule, "{709126cb-6241-4992-85a1-ef930ee5c0b3}", AWSLogExplorerModuleInterface);
        AZ_CLASS_ALLOCATOR(AWSLogExplorerModule, AZ::SystemAllocator, 0);
    };
}// namespace AWSLogExplorer

AZ_DECLARE_MODULE_CLASS(Gem_AWSLogExplorer, AWSLogExplorer::AWSLogExplorerModule)
