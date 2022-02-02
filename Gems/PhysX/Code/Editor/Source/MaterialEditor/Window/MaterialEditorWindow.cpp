/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

//#include <Atom/RHI/Factory.h>
#include <Editor/Source/PhysXMaterial/PhysXMaterialSourceData.h>
#include <Editor/Source/PhysXMaterial/PhysXMaterialTypeSourceData.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <Editor/Source/MaterialEditor/Document/MaterialDocumentRequestBus.h>
#include <Editor/Source/MaterialEditor/Window/CreateMaterialDialog/CreateMaterialDialog.h>
#include <Editor/Source/MaterialEditor/Window/MaterialEditorWindow.h>
#include <Editor/Source/MaterialEditor/Window/MaterialEditorWindowSettings.h>
#include <Editor/Source/MaterialEditor/Window/MaterialInspector/MaterialInspector.h>
#include <Editor/Source/MaterialEditor/Window/SettingsDialog/MaterialEditorSettingsDialog.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QUrl>
#include <QWindow>
AZ_POP_DISABLE_WARNING

namespace PhysX
{
    MaterialEditorWindow::MaterialEditorWindow(QWidget* parent /* = 0 */)
        : Base(parent)
    {
        resize(1280, 1024);

        // Among other things, we need the window wrapper to save the main window size, position, and state
        auto mainWindowWrapper =
            new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        mainWindowWrapper->setGuest(this);
        mainWindowWrapper->enableSaveRestoreGeometry("O3DE", "PhysXMaterialEditor", "mainWindowGeometry");

        // set the style sheet for RPE highlighting and other styling
        AzQtComponents::StyleManager::setStyleSheet(this, QStringLiteral(":/MaterialEditor.qss"));

        QApplication::setWindowIcon(QIcon(":/Icons/materialeditor.svg"));

        /*AZ::Name apiName = AZ::RHI::Factory::Get().GetName();
        if (!apiName.IsEmpty())
        {
            QString title = QString{ "%1 (%2)" }.arg(QApplication::applicationName()).arg(apiName.GetCStr());
            setWindowTitle(title);
        }
        else
        {
            AZ_Assert(false, "Render API name not found");
            setWindowTitle(QApplication::applicationName());
        }*/
        setWindowTitle(QApplication::applicationName());

        setObjectName("PhysXMaterialEditorWindow");

        //m_assetBrowser->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->SetFilterState("", PhysXMaterialAsset::Group, true);
        m_assetBrowser->SetOpenHandler([](const AZStd::string& absolutePath) {
            if (AzFramework::StringFunc::Path::IsExtension(absolutePath.c_str(), PhysXMaterialSourceData::Extension))
            {
                AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
                    &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::OpenDocument, absolutePath);
                return;
            }

            if (AzFramework::StringFunc::Path::IsExtension(absolutePath.c_str(), PhysXMaterialTypeSourceData::Extension))
            {
                return;
            }

            QDesktopServices::openUrl(QUrl::fromLocalFile(absolutePath.c_str()));
        });

        AddDockWidget("Inspector", new MaterialInspector, Qt::RightDockWidgetArea, Qt::Vertical);

        // Restore geometry and show the window
        mainWindowWrapper->showFromSettings();

        // Restore additional state for docked windows
        auto windowSettings = AZ::UserSettings::CreateFind<MaterialEditorWindowSettings>(
            AZ::Crc32("MaterialEditorWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        if (!windowSettings->m_mainWindowState.empty())
        {
            QByteArray windowState(windowSettings->m_mainWindowState.data(), static_cast<int>(windowSettings->m_mainWindowState.size()));
            m_advancedDockManager->restoreState(windowState);
        }

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    void MaterialEditorWindow::ResizeViewportRenderTarget([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
    {
    }

    void MaterialEditorWindow::LockViewportRenderTargetSize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height)
    {
    }

    void MaterialEditorWindow::UnlockViewportRenderTargetSize()
    {
    }

    bool MaterialEditorWindow::GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath)
    {
        CreateMaterialDialog createDialog(this);
        createDialog.adjustSize();

        if (createDialog.exec() == QDialog::Accepted &&
            !createDialog.m_materialFileInfo.absoluteFilePath().isEmpty() &&
            !createDialog.m_materialTypeFileInfo.absoluteFilePath().isEmpty())
        {
            savePath = createDialog.m_materialFileInfo.absoluteFilePath().toUtf8().constData();
            openPath = createDialog.m_materialTypeFileInfo.absoluteFilePath().toUtf8().constData();
            return true;
        }
        return false;
    }

    bool MaterialEditorWindow::GetOpenDocumentParams(AZStd::string& openPath)
    {
        const AZStd::vector<AZ::Data::AssetType> assetTypes = { azrtti_typeid<PhysXMaterialAsset>() };
        openPath = AtomToolsFramework::GetOpenFileInfo(assetTypes).absoluteFilePath().toUtf8().constData();
        return !openPath.empty();
    }

    void MaterialEditorWindow::OpenSettings()
    {
        SettingsDialog dialog(this);
        dialog.exec();
    }

    void MaterialEditorWindow::OpenHelp()
    {
        QMessageBox::information(
            this, windowTitle(),
            R"(<html><head/><body>
            <p><h3><u>PhysX Material Editor Controls</u></h3></p>
            </body></html>)");
    }

    void MaterialEditorWindow::OpenAbout()
    {
        QMessageBox::about(this, windowTitle(), QApplication::applicationName());
    }

    void MaterialEditorWindow::closeEvent(QCloseEvent* closeEvent)
    {
        // Capture docking state before shutdown
        auto windowSettings = AZ::UserSettings::CreateFind<MaterialEditorWindowSettings>(
            AZ::Crc32("MaterialEditorWindowSettings"), AZ::UserSettings::CT_GLOBAL);

        QByteArray windowState = m_advancedDockManager->saveState();
        windowSettings->m_mainWindowState.assign(windowState.begin(), windowState.end());

        Base::closeEvent(closeEvent);
    }
} // namespace PhysX

#include <Editor/Source/MaterialEditor/Window/moc_MaterialEditorWindow.cpp>
