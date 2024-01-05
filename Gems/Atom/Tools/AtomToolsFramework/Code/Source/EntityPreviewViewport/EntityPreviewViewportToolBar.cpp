/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>
#include <Atom/RPI.Reflect/System/RenderPipelineDescriptor.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsRequestBus.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportToolBar.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/std/containers/vector.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/Widgets/ToolBar.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/SourceThumbnail.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/Thumbnails/ThumbnailWidget.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

#include <QAbstractItemView>
#include <QAction>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>

namespace AtomToolsFramework
{
    EntityPreviewViewportToolBar::EntityPreviewViewportToolBar(const AZ::Crc32& toolId, QWidget* parent)
        : QToolBar(parent)
        , m_toolId(toolId)
    {
        setObjectName("EntityPreviewViewportToolBar");

        AzQtComponents::ToolBar::addMainToolBarStyle(this);

        // Add show grid button
        m_showGrid = addAction(QIcon(":/Icons/grid.svg"), "Show Grid");
        m_showGrid->setCheckable(true);
        connect(m_showGrid, &QAction::triggered, [this]() {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetGridEnabled, m_showGrid->isChecked());
        });

        // Add show shadow catcher button
        m_showShadowCatcher = addAction(QIcon(":/Icons/shadow.svg"), "Show Shadow Catcher");
        m_showShadowCatcher->setCheckable(true);
        connect(m_showShadowCatcher, &QAction::triggered, [this]() {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetShadowCatcherEnabled, m_showShadowCatcher->isChecked());
        });

        // Add show alternate skybox button
        m_showAlternateSkybox = addAction(QIcon(":/Icons/skybox.svg"), "Show Alternate Skybox");
        m_showAlternateSkybox->setCheckable(true);
        connect(m_showAlternateSkybox, &QAction::triggered, [this]() {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetAlternateSkyboxEnabled, m_showAlternateSkybox->isChecked());
        });

        // Add mapping selection button
        auto displayMapperAction = addAction(QIcon(":/Icons/toneMapping.svg"), "Tone Mapping", this, [this]() {
            QSharedPointer<QMenu> menu(CreateToneMappingMenu());
            menu->exec(QCursor::pos());
        });
        displayMapperAction->setCheckable(false);

        // Spacer
        auto spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        addWidget(spacer);

        auto viewOptionsMenu = new QMenu(this);
        QMenu::connect(
            viewOptionsMenu,
            &QMenu::aboutToShow,
            [this, viewOptionsMenu]()
            {
                viewOptionsMenu->clear();
                viewOptionsMenu->addAction(m_showGrid);
                viewOptionsMenu->addAction(m_showShadowCatcher);
                viewOptionsMenu->addAction(m_showAlternateSkybox);
                viewOptionsMenu->addMenu(CreateToneMappingMenu());
                viewOptionsMenu->addMenu(CreateRenderPipelineMenu());
                viewOptionsMenu->addMenu(CreateLightingPresetMenu());
                viewOptionsMenu->addMenu(CreateModelPresetMenu());
            });

        auto viewOptionButton = new QToolButton(this);
        viewOptionButton->setAutoExclusive(false);
        viewOptionButton->setIcon(QIcon(":/Icons/menu.svg"));
        viewOptionButton->setPopupMode(QToolButton::InstantPopup);
        viewOptionButton->setMenu(viewOptionsMenu);
        addWidget(viewOptionButton);

        OnViewportSettingsChanged();
        EntityPreviewViewportSettingsNotificationBus::Handler::BusConnect(m_toolId);
    }

    EntityPreviewViewportToolBar::~EntityPreviewViewportToolBar()
    {
        EntityPreviewViewportSettingsNotificationBus::Handler::BusDisconnect();
    }

    void EntityPreviewViewportToolBar::OnViewportSettingsChanged()
    {
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                m_showGrid->setChecked(viewportRequests->GetGridEnabled());
                m_showShadowCatcher->setChecked(viewportRequests->GetShadowCatcherEnabled());
                m_showAlternateSkybox->setChecked(viewportRequests->GetAlternateSkyboxEnabled());
            });
    }

    QAction* EntityPreviewViewportToolBar::CreateThumbnailWidgetAction(
        QWidget* parent,
        const AZStd::string& title,
        const AZStd::string& path,
        bool checkable,
        bool checked,
        const AZStd::function<void()>& handler) const
    {
        bool result = false;
        AZ::Data::AssetInfo assetInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            result, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, path.c_str(), assetInfo, watchFolder);

        auto thumbnailKey = MAKE_TKEY(AzToolsFramework::AssetBrowser::SourceThumbnailKey, assetInfo.m_assetId.m_guid);

        auto thumbnailParent = new QWidget(parent);

        QIcon checkMark(":/stylesheet/img/UI20/checkmark-menu.svg");
        auto thumbnailCheckBox = new QLabel(thumbnailParent);
        thumbnailCheckBox->setFixedSize(QSize(16, 16));
        thumbnailCheckBox->setPixmap(checkMark.pixmap(16));
        thumbnailCheckBox->setVisible(checkable && checked);
        QSizePolicy sp = thumbnailCheckBox->sizePolicy();
        sp.setRetainSizeWhenHidden(true);
        thumbnailCheckBox->setSizePolicy(sp);

        auto thumbnailWidget = new AzToolsFramework::Thumbnailer::ThumbnailWidget(thumbnailParent);
        thumbnailWidget->setFixedSize(QSize(32, 32));
        thumbnailWidget->SetThumbnailKey(thumbnailKey);

        auto thumbnailLabel = new QLabel(thumbnailParent);
        thumbnailLabel->setText(title.c_str());

        auto thumbnailLayout = new QHBoxLayout(thumbnailParent);
        thumbnailLayout->setContentsMargins(1, 1, 1, 1);
        thumbnailLayout->addWidget(thumbnailCheckBox);
        thumbnailLayout->addWidget(thumbnailWidget);
        thumbnailLayout->addWidget(thumbnailLabel);
        thumbnailParent->setLayout(thumbnailLayout);

        auto widgetAction = new QWidgetAction(parent);
        widgetAction->setDefaultWidget(thumbnailParent);

        AzQtComponents::Style::addClass(thumbnailParent, "WidgetAction");

        QObject::connect(
            widgetAction,
            &QAction::setChecked,
            thumbnailCheckBox,
            [thumbnailCheckBox, checkable](bool checked)
            {
                thumbnailCheckBox->setVisible(checkable && checked);
            });

        QObject::connect(widgetAction, &QAction::triggered, parent, handler);
        return widgetAction;
    }

    QMenu* EntityPreviewViewportToolBar::CreateToneMappingMenu() const
    {
        auto menu = new QMenu(tr("Tone Mapping"));
        menu->setIcon(QIcon(":/Icons/toneMapping.svg"));

        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this, menu](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                for (const auto& operationEnumPair : AZ::AzEnumTraits<AZ::Render::DisplayMapperOperationType>::Members)
                {
                    auto operationAction = menu->addAction(
                        operationEnumPair.m_string.data(),
                        [this, operationEnumPair]()
                        {
                            EntityPreviewViewportSettingsRequestBus::Event(
                                m_toolId,
                                &EntityPreviewViewportSettingsRequestBus::Events::SetDisplayMapperOperationType,
                                operationEnumPair.m_value);
                        });
                    operationAction->setCheckable(true);
                    operationAction->setChecked(operationEnumPair.m_value == viewportRequests->GetDisplayMapperOperationType());
                }
            });

        return menu;
    }

    QMenu* EntityPreviewViewportToolBar::CreateRenderPipelineMenu() const
    {
        auto menu = new QMenu(tr("Render Pipeline"));

        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this, menu](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                AZStd::map<AZStd::string, AZStd::string> sortedRenderPipelinePaths;
                for (const auto& path : viewportRequests->GetRegisteredRenderPipelinePaths())
                {
                    const auto& pathWithoutAlias = GetPathWithoutAlias(path);
                    const auto& title = GetDisplayNameFromPath(pathWithoutAlias);
                    sortedRenderPipelinePaths.emplace(title, pathWithoutAlias);
                }

                for (const auto& [title, pathWithoutAlias] : sortedRenderPipelinePaths)
                {
                    if (QFileInfo::exists(pathWithoutAlias.c_str()))
                    {
                        auto action = menu->addAction(
                            title.c_str(),
                            [this, pathWithoutAlias]()
                            {
                                EntityPreviewViewportSettingsRequestBus::Event(
                                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadRenderPipeline, pathWithoutAlias);
                            });
                        action->setCheckable(true);
                        action->setChecked(pathWithoutAlias == viewportRequests->GetLastRenderPipelinePathWithoutAlias());
                    }
                }
            });

        return menu;
    }

    QMenu* EntityPreviewViewportToolBar::CreateLightingPresetMenu() const
    {
        auto menu = new QMenu(tr("Lighting Preset"));

        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this, menu](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                AZStd::map<AZStd::string, AZStd::string> sortedLightingPresetPaths;
                for (const auto& path : viewportRequests->GetRegisteredLightingPresetPaths())
                {
                    const auto& pathWithoutAlias = GetPathWithoutAlias(path);
                    const auto& title = GetDisplayNameFromPath(pathWithoutAlias);
                    sortedLightingPresetPaths.emplace(title, pathWithoutAlias);
                }

                for (const auto& [title, pathWithoutAlias] : sortedLightingPresetPaths)
                {
                    if (QFileInfo::exists(pathWithoutAlias.c_str()))
                    {
                        auto action = CreateThumbnailWidgetAction(
                            menu,
                            title,
                            pathWithoutAlias,
                            true,
                            pathWithoutAlias == viewportRequests->GetLastLightingPresetPathWithoutAlias(),
                            [this, pathWithoutAlias]()
                            {
                                EntityPreviewViewportSettingsRequestBus::Event(
                                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset, pathWithoutAlias);
                            });
                        menu->addAction(action);
                    }
                }
            });

        return menu;
    }

    QMenu* EntityPreviewViewportToolBar::CreateModelPresetMenu() const
    {
        auto menu = new QMenu(tr("Model Preset"));

        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this, menu](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                AZStd::map<AZStd::string, AZStd::string> sortedModelPresetPaths;
                for (const auto& path : viewportRequests->GetRegisteredModelPresetPaths())
                {
                    const auto& pathWithoutAlias = GetPathWithoutAlias(path);
                    const auto& title = GetDisplayNameFromPath(pathWithoutAlias);
                    sortedModelPresetPaths.emplace(title, pathWithoutAlias);
                }

                for (const auto& [title, pathWithoutAlias] : sortedModelPresetPaths)
                {
                    if (QFileInfo::exists(pathWithoutAlias.c_str()))
                    {
                        auto action = CreateThumbnailWidgetAction(
                            menu,
                            title,
                            pathWithoutAlias,
                            true,
                            pathWithoutAlias == viewportRequests->GetLastModelPresetPathWithoutAlias(),
                            [this, pathWithoutAlias]()
                            {
                                EntityPreviewViewportSettingsRequestBus::Event(
                                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset, pathWithoutAlias);
                            });
                        menu->addAction(action);
                    }
                }

                if (!menu->actions().empty())
                {
                    menu->addSeparator();
                }

                auto importModelAction = menu->addAction(
                    tr("Import Model..."),
                    [this]()
                    {
                        const bool multiselect = false;
                        const bool supportSelectingSources = false;
                        auto selectionModel = AssetSelectionModel::AssetTypeSelection(
                            { AZ::RPI::ModelAsset::RTTI_Type() }, multiselect, supportSelectingSources);
                        selectionModel.SetTitle("Select Model");

                        AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::Broadcast(
                            &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::Events::PickAssets,
                            selectionModel,
                            GetToolMainWindow());

                        if (selectionModel.IsValid())
                        {
                            const auto entry = selectionModel.GetResult();
                            if (const auto product = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>(entry))
                            {
                                AZ::Render::ModelPreset preset;
                                preset.m_modelAsset.Create(product->GetAssetId());
                                EntityPreviewViewportSettingsRequestBus::Event(
                                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetModelPreset, preset);

                                AZStd::string outputPath = AZ::RPI::AssetUtils::GetSourcePathByAssetId(product->GetAssetId());
                                AZ::StringFunc::Path::ReplaceExtension(outputPath, AZ::Render::ModelPreset::Extension);
                                EntityPreviewViewportSettingsRequestBus::Event(
                                    m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SaveModelPreset, outputPath);
                            }
                        }
                    });
                importModelAction->setCheckable(false);
            });

        return menu;
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/EntityPreviewViewport/moc_EntityPreviewViewportToolBar.cpp>
