
#include <OceanModuleInterface.h>
#include <OceanEditorSystemComponent.h>

namespace Ocean
{
    class OceanEditorModule
        : public OceanModuleInterface
    {
    public:
        AZ_RTTI(OceanEditorModule, "{674ba7f5-0a90-49fb-adba-d918c3b24625}", OceanModuleInterface);
        AZ_CLASS_ALLOCATOR(OceanEditorModule, AZ::SystemAllocator, 0);

        OceanEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                OceanEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<OceanEditorSystemComponent>(),
            };
        }
    };
}// namespace Ocean

AZ_DECLARE_MODULE_CLASS(Gem_Ocean, Ocean::OceanEditorModule)
