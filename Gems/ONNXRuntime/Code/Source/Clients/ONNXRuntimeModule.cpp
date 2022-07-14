

#include <ONNXRuntimeModuleInterface.h>
#include "ONNXRuntimeSystemComponent.h"

namespace ONNXRuntime
{
    class ONNXRuntimeModule
        : public ONNXRuntimeModuleInterface
    {
    public:
        AZ_RTTI(ONNXRuntimeModule, "{36EC0A78-6DD6-42A9-AD2D-FB43A15F7316}", ONNXRuntimeModuleInterface);
        AZ_CLASS_ALLOCATOR(ONNXRuntimeModule, AZ::SystemAllocator, 0);
    };
}// namespace ONNXRuntime

AZ_DECLARE_MODULE_CLASS(Gem_ONNXRuntime, ONNXRuntime::ONNXRuntimeModule)
