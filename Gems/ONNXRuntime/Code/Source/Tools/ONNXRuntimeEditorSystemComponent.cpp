
#include <AzCore/Serialization/SerializeContext.h>
#include "ONNXRuntimeEditorSystemComponent.h"

namespace ONNXRuntime
{
    void ONNXRuntimeEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ONNXRuntimeEditorSystemComponent, ONNXRuntimeSystemComponent>()
                ->Version(0);
        }
    }

    ONNXRuntimeEditorSystemComponent::ONNXRuntimeEditorSystemComponent() = default;

    ONNXRuntimeEditorSystemComponent::~ONNXRuntimeEditorSystemComponent() = default;

    void ONNXRuntimeEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("ONNXRuntimeEditorService"));
    }

    void ONNXRuntimeEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("ONNXRuntimeEditorService"));
    }

    void ONNXRuntimeEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void ONNXRuntimeEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void ONNXRuntimeEditorSystemComponent::Activate()
    {
        ONNXRuntimeSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ONNXRuntimeEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ONNXRuntimeSystemComponent::Deactivate();
    }

} // namespace ONNXRuntime
