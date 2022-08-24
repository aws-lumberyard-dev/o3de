
#pragma once

#include <Components/CloudsSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace Clouds
{
    /// System component for Clouds editor
    class CloudsEditorSystemComponent
        : public CloudsSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = CloudsSystemComponent;
    public:
        AZ_COMPONENT(CloudsEditorSystemComponent, "{B57E9601-4BD9-487C-8C4A-C93D52C4DF76}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        CloudsEditorSystemComponent();
        ~CloudsEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace Clouds
