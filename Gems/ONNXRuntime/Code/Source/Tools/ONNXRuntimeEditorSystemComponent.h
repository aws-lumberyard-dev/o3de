
#pragma once

#include <Clients/ONNXRuntimeSystemComponent.h>

namespace ONNXRuntime
{
    /// System component for ONNXRuntime editor
    class ONNXRuntimeEditorSystemComponent
        : public ONNXRuntimeSystemComponent
    {
        using BaseSystemComponent = ONNXRuntimeSystemComponent;
    public:
        AZ_COMPONENT(ONNXRuntimeEditorSystemComponent, "{D8218124-0EF5-4336-90D3-FC3545681352}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        ONNXRuntimeEditorSystemComponent();
        ~ONNXRuntimeEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace ONNXRuntime
