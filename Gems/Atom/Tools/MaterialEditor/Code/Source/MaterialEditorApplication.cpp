/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Document/CreateDocumentDialog.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <MaterialEditorApplication.h>

#include <Document/MaterialDocument.h>
#include <Window/MaterialEditorMainWindow.h>

void InitMaterialEditorResources()
{
    // Must register qt resources from other modules
    Q_INIT_RESOURCE(MaterialEditor);
    Q_INIT_RESOURCE(InspectorWidget);
    Q_INIT_RESOURCE(AtomToolsAssetBrowser);
}

namespace MaterialEditor
{
    static const char* GetBuildTargetName()
    {
#if !defined(LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return LY_CMAKE_TARGET;
    }

    MaterialEditorApplication::MaterialEditorApplication(int* argc, char*** argv)
        : Base(GetBuildTargetName(), argc, argv)
    {
        InitMaterialEditorResources();

        QApplication::setOrganizationName("O3DE");
        QApplication::setApplicationName("O3DE Material Editor");
        QApplication::setWindowIcon(QIcon(":/Icons/application.svg"));

        AzToolsFramework::EditorWindowRequestBus::Handler::BusConnect();
    }

    MaterialEditorApplication::~MaterialEditorApplication()
    {
        AzToolsFramework::EditorWindowRequestBus::Handler::BusDisconnect();
        m_window.reset();
    }

    void MaterialEditorApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);
        MaterialDocument::Reflect(context);
    }

    const char* MaterialEditorApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialEditor";
#elif defined(_DEBUG)
        return "DebugMaterialEditor";
#else
        return "ProfileMaterialEditor";
#endif
    }

    void MaterialEditorApplication::StartCommon(AZ::Entity* systemEntity)
    {
        Base::StartCommon(systemEntity);

        // Overriding default document type info to provide a custom view
        auto documentTypeInfo = MaterialDocument::BuildDocumentTypeInfo();
        documentTypeInfo.m_documentViewFactoryCallback = [this]([[maybe_unused]] const AZ::Crc32& toolId, const AZ::Uuid& documentId) {
            auto emptyWidget = new QWidget(m_window.get());
            emptyWidget->setContentsMargins(0, 0, 0, 0);
            emptyWidget->setFixedSize(0, 0);
            return m_window->AddDocumentTab(documentId, emptyWidget);
        };
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType, documentTypeInfo);

        m_viewportSettingsSystem.reset(aznew AtomToolsFramework::EntityPreviewViewportSettingsSystem(m_toolId));

        m_window.reset(aznew MaterialEditorMainWindow(m_toolId));
        m_window->show();

        if (m_commandLine.HasSwitch("create-path"))
        {
            using namespace AtomToolsFramework;
            const auto& savePath = m_commandLine.GetSwitchValue("create-path", 0);
            if (!savePath.empty())
            {
                DocumentTypeInfoVector documentTypes;
                AtomToolsDocumentSystemRequestBus::EventResult(
                    documentTypes, m_toolId, &AtomToolsDocumentSystemRequestBus::Events::GetRegisteredDocumentTypes);
                const DocumentTypeInfo& documentType = documentTypes[0];
                CreateDocumentDialog dialog(documentType, savePath.c_str(), GetAppMainWindow());
                dialog.adjustSize();
                if (dialog.exec() == QDialog::Accepted && !dialog.m_sourcePath.isEmpty() && !dialog.m_targetPath.isEmpty())
                {
                    AtomToolsDocumentSystemRequestBus::Event(
                        m_toolId, &AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromFilePath,
                        dialog.m_sourcePath.toUtf8().constData(), dialog.m_targetPath.toUtf8().constData());
                }
            }
        }
    }

    void MaterialEditorApplication::Destroy()
    {
        m_window.reset();
        m_viewportSettingsSystem.reset();
        Base::Destroy();
    }

    AZStd::vector<AZStd::string> MaterialEditorApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({ "passes/", "config/", "MaterialEditor/" });
    }

    QWidget* MaterialEditorApplication::GetAppMainWindow()
    {
        return m_window.get();
    }
} // namespace MaterialEditor
