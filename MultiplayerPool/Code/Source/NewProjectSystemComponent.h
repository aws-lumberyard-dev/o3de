
#pragma once

#include <AzCore/Component/Component.h>

#include <NewProject/NewProjectBus.h>

namespace NewProject
{
    class NewProjectSystemComponent
        : public AZ::Component
        , protected NewProjectRequestBus::Handler
    {
    public:
        AZ_COMPONENT(NewProjectSystemComponent, "{50f54e3e-dd27-49f9-a430-067ce6689da1}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        NewProjectSystemComponent();
        ~NewProjectSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // NewProjectRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
