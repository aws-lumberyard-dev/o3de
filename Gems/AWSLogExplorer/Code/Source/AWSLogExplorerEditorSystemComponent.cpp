
#include <AzCore/Serialization/SerializeContext.h>
#include <AWSLogExplorerEditorSystemComponent.h>

namespace AWSLogExplorer
{
    void AWSLogExplorerEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSLogExplorerEditorSystemComponent, AWSLogExplorerSystemComponent>()
                ->Version(0);
        }
    }

    AWSLogExplorerEditorSystemComponent::AWSLogExplorerEditorSystemComponent() = default;

    AWSLogExplorerEditorSystemComponent::~AWSLogExplorerEditorSystemComponent() = default;

    void AWSLogExplorerEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("AWSLogExplorerEditorService"));
    }

    void AWSLogExplorerEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("AWSLogExplorerEditorService"));
    }

    void AWSLogExplorerEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void AWSLogExplorerEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void AWSLogExplorerEditorSystemComponent::Activate()
    {
        AWSLogExplorerSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void AWSLogExplorerEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AWSLogExplorerSystemComponent::Deactivate();
    }

} // namespace AWSLogExplorer
