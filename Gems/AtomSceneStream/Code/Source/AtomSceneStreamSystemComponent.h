
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AtomSceneStream/AtomSceneStreamBus.h>

namespace AZ
{
    namespace AtomSceneStream
    {
        class AtomSceneStreamSystemComponent
            : public AZ::Component
            , protected AtomSceneStreamRequestBus::Handler
            , public AZ::TickBus::Handler
        {
        public:
            AZ_COMPONENT(AtomSceneStreamSystemComponent, "{e0ac9825-4b03-4789-8dc1-1894e67122a2}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            AtomSceneStreamSystemComponent();
            ~AtomSceneStreamSystemComponent();

        protected:
            ////////////////////////////////////////////////////////////////////////
            // AtomSceneStreamRequestBus interface implementation

            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            // AZTickBus interface implementation
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            ////////////////////////////////////////////////////////////////////////
        };

    } // namespace AtomSceneStream

}
