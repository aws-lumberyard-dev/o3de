
#pragma once

#include <OceanSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace Ocean
{
    /// System component for Ocean editor
    class OceanEditorSystemComponent
        : public OceanSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = OceanSystemComponent;
    public:
        AZ_COMPONENT(OceanEditorSystemComponent, "{797a10fe-973e-4525-a663-1db514b9560f}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        OceanEditorSystemComponent();
        ~OceanEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace Ocean
