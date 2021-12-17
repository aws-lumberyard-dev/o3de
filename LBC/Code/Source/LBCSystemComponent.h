
#pragma once

#include <AzCore/Component/Component.h>
#include <LBC/LBCBus.h>

namespace LBC
{
    class LBCSystemComponent
        : public AZ::Component
        , public LBCRequestBus::Handler
    {
    public:
        AZ_COMPONENT(LBCSystemComponent, "{a5e77d2d-c2b0-4a46-a4a9-bbdecb2a437a}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        LBCSystemComponent();
        ~LBCSystemComponent();

        void SetRedBallEntityId(AZ::EntityId entityId) override;
        AZ::EntityId GetRedBallEntityId() const override;
        void SetBlueBallEntityId(AZ::EntityId entityId) override;
        AZ::EntityId GetBlueBallEntityId() const override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LBCRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZ::EntityId m_redBallEntityId = AZ::EntityId();
        AZ::EntityId m_blueBallEntityId = AZ::EntityId();
    };
}
