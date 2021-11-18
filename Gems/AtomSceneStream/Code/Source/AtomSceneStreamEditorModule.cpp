
#include <AtomSceneStreamModuleInterface.h>
#include <AtomSceneStreamEditorSystemComponent.h>

namespace AZ
{
    namespace AtomSceneStream
    {
        class AtomSceneStreamEditorModule
            : public AtomSceneStreamModuleInterface
        {
        public:
            AZ_RTTI(AtomSceneStreamEditorModule, "{37a6ac37-5b31-4431-b01b-41606b1386b5}", AtomSceneStreamModuleInterface);
            AZ_CLASS_ALLOCATOR(AtomSceneStreamEditorModule, AZ::SystemAllocator, 0);

            AtomSceneStreamEditorModule()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                // Add ALL components descriptors associated with this gem to m_descriptors.
                // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
                // This happens through the [MyComponent]::Reflect() function.
                m_descriptors.insert(m_descriptors.end(), {
                    AtomSceneStreamEditorSystemComponent::CreateDescriptor(),
                });
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             * Non-SystemComponents should not be added here
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return AZ::ComponentTypeList {
                    azrtti_typeid<AtomSceneStreamEditorSystemComponent>(),
                };
            }
        };
    } // namespace AtomSceneStream

    AZ_DECLARE_MODULE_CLASS(Gem_AtomSceneStream, AtomSceneStream::AtomSceneStreamEditorModule)
}

