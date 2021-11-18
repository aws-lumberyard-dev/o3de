
#pragma once

#include <AtomSceneStreamSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AZ
{
    namespace AtomSceneStream
    {
        /// System component for AtomSceneStream editor
        class AtomSceneStreamEditorSystemComponent
            : public AtomSceneStreamSystemComponent
            , private AzToolsFramework::EditorEvents::Bus::Handler
        {
            using BaseSystemComponent = AtomSceneStreamSystemComponent;
        public:
            AZ_COMPONENT(AtomSceneStreamEditorSystemComponent, "{73bb1cf7-9d57-43e3-91fe-8a56b4dd16c9}", BaseSystemComponent);
            static void Reflect(AZ::ReflectContext* context);

            AtomSceneStreamEditorSystemComponent();
            ~AtomSceneStreamEditorSystemComponent();

        private:
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            // AZ::Component
            void Activate() override;
            void Deactivate() override;
        };
    } // namespace AtomSceneStream
}

