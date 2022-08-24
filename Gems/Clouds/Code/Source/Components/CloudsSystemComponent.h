
#pragma once

#include <AzCore/Component/Component.h>
#include <Clouds/CloudsBus.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

namespace Clouds
{
    class CloudsSystemComponent
        : public AZ::Component
        , protected CloudsRequestBus::Handler
    {
    public:
        AZ_COMPONENT(CloudsSystemComponent, "{A83E59CB-1A27-46DA-8AF8-6A8006E26CA8}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        CloudsSystemComponent();
        ~CloudsSystemComponent();

    protected:
        //! CloudsRequestBus interface implementation

        //! AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;

    private:
        //! Used for loading the pass templates
        AZ::RPI::PassSystemInterface::OnReadyLoadTemplatesEvent::Handler m_loadTemplatesHandler;
    };

} // namespace Clouds
