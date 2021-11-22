
#include <AtomSceneStreamModuleInterface.h>
#include <AtomSceneStreamEditorSystemComponent.h>

#pragma optimize("", off)

namespace AZ
{
    namespace AtomSceneStream
    {
        class AtomSceneStreamEditorModule
            : public AtomSceneStreamModuleInterface
        {
        public:
            AZ_RTTI(AtomSceneStreamEditorModule, "{88FB772B-2C06-43E6-A56C-01E4F84C0326}", AtomSceneStreamModuleInterface);
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

    AZ_DECLARE_MODULE_CLASS(Gem_AtomSceneStream, AZ::AtomSceneStream::AtomSceneStreamEditorModule)
}

#pragma optimize("", on)

