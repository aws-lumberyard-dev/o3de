
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <OceanSystemComponent.h>

namespace Ocean
{
    class OceanModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(OceanModuleInterface, "{d9b8fef1-a697-4a1c-a589-d2648debb9c7}", AZ::Module);
        AZ_CLASS_ALLOCATOR(OceanModuleInterface, AZ::SystemAllocator, 0);

        OceanModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                OceanSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<OceanSystemComponent>(),
            };
        }
    };
}// namespace Ocean
