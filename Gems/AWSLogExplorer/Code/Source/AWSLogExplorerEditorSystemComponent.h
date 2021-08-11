
#pragma once

#include <AWSLogExplorerSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AWSLogExplorer
{
    /// System component for AWSLogExplorer editor
    class AWSLogExplorerEditorSystemComponent
        : public AWSLogExplorerSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = AWSLogExplorerSystemComponent;
    public:
        AZ_COMPONENT(AWSLogExplorerEditorSystemComponent, "{a172ea20-9b5c-4be5-917e-167791ed496c}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        AWSLogExplorerEditorSystemComponent();
        ~AWSLogExplorerEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace AWSLogExplorer
