
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Components/CloudsSystemComponent.h>
#include <Components/CloudsComponent.h>

namespace Clouds
{
    class CloudsModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(CloudsModuleInterface, "{D26C1B35-897F-4716-A8AE-7788B2CA98C5}", AZ::Module);
        AZ_CLASS_ALLOCATOR(CloudsModuleInterface, AZ::SystemAllocator, 0);

        CloudsModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                CloudsSystemComponent::CreateDescriptor(),
                CloudsComponent::CreateDescriptor()
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudsSystemComponent>(),
            };
        }
    };
}// namespace Clouds
