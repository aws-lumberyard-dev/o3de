
#include <CloudsModuleInterface.h>
#include <Components/CloudsEditorSystemComponent.h>
#include <Components/CloudsEditorComponent.h>

namespace Clouds
{
    class CloudsEditorModule
        : public CloudsModuleInterface
    {
    public:
        AZ_RTTI(CloudsEditorModule, "{967C196F-C37C-4C80-9D42-F3A507BD8BD7}", CloudsModuleInterface);
        AZ_CLASS_ALLOCATOR(CloudsEditorModule, AZ::SystemAllocator, 0);

        CloudsEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                CloudsEditorSystemComponent::CreateDescriptor(),
                CloudsEditorComponent::CreateDescriptor()
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<CloudsEditorSystemComponent>(),
            };
        }
    };
}// namespace Clouds

AZ_DECLARE_MODULE_CLASS(Gem_Clouds, Clouds::CloudsEditorModule)
