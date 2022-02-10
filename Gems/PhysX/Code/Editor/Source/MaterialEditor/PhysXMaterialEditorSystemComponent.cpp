
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>

#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/AssetBrowser/AtomToolsAssetBrowserInteractions.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>

#include <Editor/Source/MaterialEditor/PhysXMaterialEditorSystemComponent.h>
#include <Editor/Source/MaterialEditor/Window/MaterialEditorWindow.h>
#include <Editor/Source/MaterialEditor/Window/MaterialEditorWindowSettings.h>
#include <Editor/Source/MaterialEditor/Window/CreateMaterialDialog/CreateMaterialDialog.h>
#include <Editor/Source/MaterialEditor/Document/MaterialDocument.h>

#include <Editor/Source/PhysXMaterial/PhysXMaterialSourceData.h>
#include <Editor/Source/PhysXMaterial/PhysXMaterialTypeSourceData.h>

#include <QDesktopServices>
#include <QDialog>
#include <QMenu>
#include <QUrl>

namespace PhysXMaterialEditor
{
    void PhysXMaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialEditorWindowSettings::Reflect(context);

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialDocumentRequestBus>("MaterialDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "physxmaterialeditor")
                ->Event("SetPropertyValue", &MaterialDocumentRequestBus::Events::SetPropertyValue)
                ->Event("GetPropertyValue", &MaterialDocumentRequestBus::Events::GetPropertyValue);
        }

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysXMaterialEditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    PhysXMaterialEditorSystemComponent::PhysXMaterialEditorSystemComponent()
        : m_notifyRegisterViewsEventHandler([this]() { RegisterPhysXWindow(); })
    {
    }

    PhysXMaterialEditorSystemComponent::~PhysXMaterialEditorSystemComponent() = default;

    void PhysXMaterialEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysXMaterialEditorService"));
    }

    void PhysXMaterialEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXMaterialEditorService"));
    }

    void PhysXMaterialEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AtomToolsDocumentSystemService"));
        required.push_back(AZ_CRC_CE("O3DEMaterialEditorService"));
    }

    void PhysXMaterialEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void PhysXMaterialEditorSystemComponent::Activate()
    {
        if (auto* o3deMaterialEditor = O3DEMaterialEditor::O3DEMaterialEditorInterface::Get())
        {
            o3deMaterialEditor->ConnectNotifyRegisterViewsEventHandler(m_notifyRegisterViewsEventHandler);
        }
    }

    void PhysXMaterialEditorSystemComponent::Deactivate()
    {
        m_notifyRegisterViewsEventHandler.Disconnect();
    }

    void PhysXMaterialEditorSystemComponent::RegisterPhysXWindow()
    {
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            []() { return aznew PhysXMaterialEditor::MaterialDocument(); });

        SetupAssetBrowserInteractions();

        O3DEMaterialEditor::RegisterViewPane<MaterialEditorWindow>("PhysX Materials");
    }

    void PhysXMaterialEditorSystemComponent::SetupAssetBrowserInteractions()
    {
        m_assetBrowserInteractions.reset(aznew AtomToolsFramework::AtomToolsAssetBrowserInteractions);
        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Source;
            },
            []([[maybe_unused]] QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                const bool isMaterial = AzFramework::StringFunc::Path::IsExtension(
                    entries.front()->GetFullPath().c_str(), AZ::PhysX::MaterialSourceData::Extension);
                const bool isMaterialType = AzFramework::StringFunc::Path::IsExtension(
                    entries.front()->GetFullPath().c_str(), AZ::PhysX::MaterialTypeSourceData::Extension);
                if (isMaterial || isMaterialType)
                {
                    menu->addAction(QObject::tr("Open"), [entries]()
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument,
                                entries.front()->GetFullPath());
                        });

                    const QString createActionName =
                        isMaterialType ? QObject::tr("Create Material...") : QObject::tr("Create Child Material...");

                    menu->addAction(createActionName, [entries]()
                        {
                            const QString defaultPath = AtomToolsFramework::GetUniqueFileInfo(
                                QString(AZ::Utils::GetProjectPath().c_str()) +
                                AZ_CORRECT_FILESYSTEM_SEPARATOR + "Assets" +
                                AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled." +
                                AZ::PhysX::MaterialSourceData::Extension).absoluteFilePath();

                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                                entries.front()->GetFullPath(),
                                AtomToolsFramework::GetSaveFileInfo(defaultPath).absoluteFilePath().toUtf8().constData());
                        });
                }
                else
                {
                    menu->addAction(QObject::tr("Open"), [entries]()
                        {
                            QDesktopServices::openUrl(QUrl::fromLocalFile(entries.front()->GetFullPath().c_str()));
                        });
                }
            });

        m_assetBrowserInteractions->RegisterContextMenuActions(
            [](const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                return entries.front()->GetEntryType() == AzToolsFramework::AssetBrowser::AssetBrowserEntry::AssetEntryType::Folder;
            },
            [](QWidget* caller, QMenu* menu, const AtomToolsFramework::AtomToolsAssetBrowserInteractions::AssetBrowserEntryVector& entries)
            {
                menu->addAction(QObject::tr("Create Material..."), [caller, entries]()
                    {
                        CreateMaterialDialog createDialog(entries.front()->GetFullPath().c_str(), caller);
                        createDialog.adjustSize();

                        if (createDialog.exec() == QDialog::Accepted &&
                            !createDialog.m_materialFileInfo.absoluteFilePath().isEmpty() &&
                            !createDialog.m_materialTypeFileInfo.absoluteFilePath().isEmpty())
                        {
                            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFile,
                                createDialog.m_materialTypeFileInfo.absoluteFilePath().toUtf8().constData(),
                                createDialog.m_materialFileInfo.absoluteFilePath().toUtf8().constData());
                        }
                    });
            });
    }

} // namespace PhysXMaterialEditor
