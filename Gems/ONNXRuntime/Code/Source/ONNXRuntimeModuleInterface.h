
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Clients/ONNXRuntimeSystemComponent.h>

namespace ONNXRuntime
{
    class ONNXRuntimeModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(ONNXRuntimeModuleInterface, "{5B79395D-DC4C-48AE-9867-302102AA2BED}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ONNXRuntimeModuleInterface, AZ::SystemAllocator, 0);

        ONNXRuntimeModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ONNXRuntimeSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ONNXRuntimeSystemComponent>(),
            };
        }
    };
}// namespace ONNXRuntime
