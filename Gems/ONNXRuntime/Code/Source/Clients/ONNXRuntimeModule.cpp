

#include <ONNXRuntimeModuleInterface.h>
#include "ONNXRuntimeSystemComponent.h"
// #include <../../3rdParty/onnxruntime-win-x64-1.11.1/include/onnxruntime_cxx_api.h>

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
