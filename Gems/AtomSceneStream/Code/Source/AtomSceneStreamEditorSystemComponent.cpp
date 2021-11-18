
#include <AzCore/Serialization/SerializeContext.h>
#include <AtomSceneStreamEditorSystemComponent.h>

namespace AZ
{
    namespace AtomSceneStream
    {
        void AtomSceneStreamEditorSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AtomSceneStreamEditorSystemComponent, AtomSceneStreamSystemComponent>()
                    ->Version(0);
            }
        }

        AtomSceneStreamEditorSystemComponent::AtomSceneStreamEditorSystemComponent() = default;

        AtomSceneStreamEditorSystemComponent::~AtomSceneStreamEditorSystemComponent() = default;

        void AtomSceneStreamEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            BaseSystemComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC_CE("AtomSceneStreamEditorService"));
        }

        void AtomSceneStreamEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            BaseSystemComponent::GetIncompatibleServices(incompatible);
            incompatible.push_back(AZ_CRC_CE("AtomSceneStreamEditorService"));
        }

        void AtomSceneStreamEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            BaseSystemComponent::GetRequiredServices(required);
        }

        void AtomSceneStreamEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            BaseSystemComponent::GetDependentServices(dependent);
        }

        void AtomSceneStreamEditorSystemComponent::Activate()
        {
            AtomSceneStreamSystemComponent::Activate();
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        }

        void AtomSceneStreamEditorSystemComponent::Deactivate()
        {
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
            AtomSceneStreamSystemComponent::Deactivate();
        }

    } // namespace AtomSceneStream

}
