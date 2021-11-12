
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <AtomSceneStreamSystemComponent.h>

namespace AtomSceneStream
{
    class AtomSceneStreamModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(AtomSceneStreamModuleInterface, "{3c9b4434-1f73-4b46-b63c-3329b3cc3f33}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AtomSceneStreamModuleInterface, AZ::SystemAllocator, 0);

        AtomSceneStreamModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                AtomSceneStreamSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AtomSceneStreamSystemComponent>(),
            };
        }
    };
}// namespace AtomSceneStream
