
#include <AWSLogExplorerModuleInterface.h>
#include <AWSLogExplorerEditorSystemComponent.h>

namespace AWSLogExplorer
{
    class AWSLogExplorerEditorModule
        : public AWSLogExplorerModuleInterface
    {
    public:
        AZ_RTTI(AWSLogExplorerEditorModule, "{709126cb-6241-4992-85a1-ef930ee5c0b3}", AWSLogExplorerModuleInterface);
        AZ_CLASS_ALLOCATOR(AWSLogExplorerEditorModule, AZ::SystemAllocator, 0);

        AWSLogExplorerEditorModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                AWSLogExplorerEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<AWSLogExplorerEditorSystemComponent>(),
            };
        }
    };
}// namespace AWSLogExplorer

AZ_DECLARE_MODULE_CLASS(Gem_AWSLogExplorer, AWSLogExplorer::AWSLogExplorerEditorModule)
