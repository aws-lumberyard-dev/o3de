
#pragma once

#include <AzCore/Component/Component.h>
#include <O3DEMaterialEditor/O3DEMaterialEditorBus.h>

namespace AtomToolsFramework
{
    class AtomToolsAssetBrowserInteractions;
    class AtomToolsDocumentSystem;
}

namespace PhysXMaterialEditor
{
    /// System component for MaterialEditor editor
    class PhysXMaterialEditorSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(PhysXMaterialEditorSystemComponent, "{6710C447-ED80-48BF-887D-89DEF461AFB5}");
        static void Reflect(AZ::ReflectContext* context);

        PhysXMaterialEditorSystemComponent();
        ~PhysXMaterialEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        void RegisterPhysXWindow();
        void SetupAssetBrowserInteractions();

        const AZStd::string m_targetName;
        const AZ::Crc32 m_toolId = {};

        O3DEMaterialEditor::O3DEMaterialEditorRequests::NotifyRegisterViewsEvent::Handler m_notifyRegisterViewsEventHandler;

        AZStd::unique_ptr<AtomToolsFramework::AtomToolsDocumentSystem> m_documentSystem;
        AZStd::unique_ptr<AtomToolsFramework::AtomToolsAssetBrowserInteractions> m_assetBrowserInteractions;
    };
} // namespace PhysX
