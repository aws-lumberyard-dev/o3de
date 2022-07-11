
#include <ONNXRuntimeModuleInterface.h>
#include "ONNXRuntimeEditorSystemComponent.h"

namespace ONNXRuntime
{
    class ONNXRuntimeEditorModule
        : public ONNXRuntimeModuleInterface
    {
    public:
        AZ_RTTI(ONNXRuntimeEditorModule, "{36EC0A78-6DD6-42A9-AD2D-FB43A15F7316}", ONNXRuntimeModuleInterface);
        AZ_CLASS_ALLOCATOR(ONNXRuntimeEditorModule, AZ::SystemAllocator, 0);

        ONNXRuntimeEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ONNXRuntimeEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<ONNXRuntimeEditorSystemComponent>(),
            };
        }
    };
}// namespace ONNXRuntime

AZ_DECLARE_MODULE_CLASS(Gem_ONNXRuntime, ONNXRuntime::ONNXRuntimeEditorModule)
