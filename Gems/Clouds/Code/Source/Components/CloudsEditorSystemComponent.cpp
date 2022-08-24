
#include <AzCore/Serialization/SerializeContext.h>
#include <Components/CloudsEditorSystemComponent.h>

namespace Clouds
{
    void CloudsEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CloudsEditorSystemComponent, CloudsSystemComponent>()
                ->Version(0);
        }
    }

    CloudsEditorSystemComponent::CloudsEditorSystemComponent() = default;

    CloudsEditorSystemComponent::~CloudsEditorSystemComponent() = default;

    void CloudsEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("CloudsEditorService"));
    }

    void CloudsEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("CloudsEditorService"));
    }

    void CloudsEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void CloudsEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void CloudsEditorSystemComponent::Activate()
    {
        CloudsSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void CloudsEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        CloudsSystemComponent::Deactivate();
    }

} // namespace Clouds
