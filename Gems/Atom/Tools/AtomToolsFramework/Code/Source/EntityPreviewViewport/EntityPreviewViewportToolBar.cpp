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
            AZ::Render::DisplayMapperOperationType currentOperationType = {};
            EntityPreviewViewportSettingsRequestBus::EventResult(
                currentOperationType, m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::GetDisplayMapperOperationType);

            QMenu menu;
            for (const auto& operationEnumPair : AZ::AzEnumTraits<AZ::Render::DisplayMapperOperationType>::Members)
            {
                auto operationAction = menu.addAction(operationEnumPair.m_string.data(), [this, operationEnumPair]() {
                    EntityPreviewViewportSettingsRequestBus::Event(
                        m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::SetDisplayMapperOperationType,
                        operationEnumPair.m_value);
                });
                operationAction->setCheckable(true);
                operationAction->setChecked(currentOperationType == operationEnumPair.m_value);
            }
            menu.exec(QCursor::pos());
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
            [this, viewOptionsMenu, toolId = m_toolId]()
            {
                viewOptionsMenu->clear();

                EntityPreviewViewportSettingsRequestBus::Event(
                    toolId,
                    [this, viewOptionsMenu, toolId](EntityPreviewViewportSettingsRequests* viewportRequests)
                    {
                        auto createThumbnailWidgetAction =
                            [](QWidget* parent, const AZStd::string& title, const AZStd::string& path, bool checkable, bool checked)
                        {
                            bool result = false;
                            AZ::Data::AssetInfo assetInfo;
                            AZStd::string watchFolder;
                            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                                result,
                                &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
                                path.c_str(),
                                assetInfo,
                                watchFolder);

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
                            return widgetAction;
                        };

                        viewOptionsMenu->addAction(m_showGrid);
                        viewOptionsMenu->addAction(m_showShadowCatcher);
                        viewOptionsMenu->addAction(m_showAlternateSkybox);

                        auto selectToneMappingMenu = viewOptionsMenu->addMenu(QIcon(":/Icons/toneMapping.svg"), tr("Tone Mapping"));
                        for (const auto& operationEnumPair : AZ::AzEnumTraits<AZ::Render::DisplayMapperOperationType>::Members)
                        {
                            auto operationAction = selectToneMappingMenu->addAction(
                                operationEnumPair.m_string.data(),
                                [toolId, operationEnumPair]()
                                {
                                    EntityPreviewViewportSettingsRequestBus::Event(
                                        toolId,
                                        &EntityPreviewViewportSettingsRequestBus::Events::SetDisplayMapperOperationType,
                                        operationEnumPair.m_value);
                                });
                            operationAction->setCheckable(true);
                            operationAction->setChecked(operationEnumPair.m_value == viewportRequests->GetDisplayMapperOperationType());
                        }

                        auto selectRenderPipelinesMenu = viewOptionsMenu->addMenu(tr("Render Pipeline"));
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
                                auto action = selectRenderPipelinesMenu->addAction(
                                    title.c_str(),
                                    [toolId, pathWithoutAlias]()
                                    {
                                        EntityPreviewViewportSettingsRequestBus::Event(
                                            toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadRenderPipeline, pathWithoutAlias);
                                    });
                                action->setCheckable(true);
                                action->setChecked(pathWithoutAlias == viewportRequests->GetLastRenderPipelinePathWithoutAlias());
                            }
                        }

                        auto selectLightingPresetsMenu = viewOptionsMenu->addMenu(tr("Lighting Preset"));
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
                                auto action = createThumbnailWidgetAction(
                                    selectLightingPresetsMenu,
                                    title,
                                    pathWithoutAlias,
                                    true,
                                    pathWithoutAlias == viewportRequests->GetLastLightingPresetPathWithoutAlias());
                                QObject::connect(
                                    action,
                                    &QAction::triggered,
                                    selectLightingPresetsMenu,
                                    [toolId, pathWithoutAlias]()
                                    {
                                        EntityPreviewViewportSettingsRequestBus::Event(
                                            toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset, pathWithoutAlias);
                                    });
                                selectLightingPresetsMenu->addAction(action);
                            }
                        }

                        auto selectModelPresetsMenu = viewOptionsMenu->addMenu(tr("Model Preset"));
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
                                auto action = createThumbnailWidgetAction(
                                    selectModelPresetsMenu,
                                    title,
                                    pathWithoutAlias,
                                    true,
                                    pathWithoutAlias == viewportRequests->GetLastModelPresetPathWithoutAlias());
                                QObject::connect(
                                    action,
                                    &QAction::triggered,
                                    selectModelPresetsMenu,
                                    [toolId, pathWithoutAlias]()
                                    {
                                        EntityPreviewViewportSettingsRequestBus::Event(
                                            toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset, pathWithoutAlias);
                                    });
                                selectModelPresetsMenu->addAction(action);
                            }
                        }
                    });
            });

        auto viewOptionButton = new QToolButton(this);
        viewOptionButton->setAutoExclusive(false);
        viewOptionButton->setIcon(QIcon(":/Icons/menu.svg"));
        viewOptionButton->setPopupMode(QToolButton::InstantPopup);
        viewOptionButton->setMenu(viewOptionsMenu);
        addWidget(viewOptionButton);

        // Setting the minimum drop down with for all asset selection combo boxes to compensate for longer file names, like render
        // pipelines.
        const int minComboBoxDropdownWidth = 220;

        // Add lighting preset combo box
        m_lightingPresetComboBox = new AssetSelectionComboBox([](const AZStd::string& path) {
            return path.ends_with(AZ::Render::LightingPreset::Extension);
        }, this);
        m_lightingPresetComboBox->view()->setMinimumWidth(minComboBoxDropdownWidth);
        m_lightingPresetComboBox->setVisible(false);
        addWidget(m_lightingPresetComboBox);

        // Add model preset combo box
        m_modelPresetComboBox = new AssetSelectionComboBox([](const AZStd::string& path) {
            return path.ends_with(AZ::Render::ModelPreset::Extension);
        }, this);
        m_modelPresetComboBox->view()->setMinimumWidth(minComboBoxDropdownWidth);
        m_modelPresetComboBox->setVisible(false);
        addWidget(m_modelPresetComboBox);

        // Add render pipeline combo box
        m_renderPipelineComboBox = new AssetSelectionComboBox([](const AZStd::string& path) {
            return path.ends_with(AZ::RPI::RenderPipelineDescriptor::Extension);
        }, this);
        m_renderPipelineComboBox->view()->setMinimumWidth(minComboBoxDropdownWidth);
        m_renderPipelineComboBox->setVisible(false);
        addWidget(m_renderPipelineComboBox);
        
        // Prepopulating preset selection widgets with previously registered presets.
        EntityPreviewViewportSettingsRequestBus::Event(
            m_toolId,
            [this](EntityPreviewViewportSettingsRequests* viewportRequests)
            {
                m_lightingPresetComboBox->AddPath(viewportRequests->GetLastLightingPresetPath());
                for (const auto& path : viewportRequests->GetRegisteredLightingPresetPaths())
                {
                    m_lightingPresetComboBox->AddPath(path);
                }

                m_modelPresetComboBox->AddPath(viewportRequests->GetLastModelPresetPath());
                for (const auto& path : viewportRequests->GetRegisteredModelPresetPaths())
                {
                    m_modelPresetComboBox->AddPath(path);
                }
                
                m_renderPipelineComboBox->AddPath(viewportRequests->GetLastRenderPipelinePath());
                for (const auto& path : viewportRequests->GetRegisteredRenderPipelinePaths())
                {
                    m_renderPipelineComboBox->AddPath(path);
                }
            });

        connect(m_lightingPresetComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadLightingPreset, path);
        });

        connect(m_modelPresetComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadModelPreset, path);
        });

        connect(m_renderPipelineComboBox, &AssetSelectionComboBox::PathSelected, this, [this](const AZStd::string& path) {
            EntityPreviewViewportSettingsRequestBus::Event(
                m_toolId, &EntityPreviewViewportSettingsRequestBus::Events::LoadRenderPipeline, path);
        });
 
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
                m_lightingPresetComboBox->SelectPath(viewportRequests->GetLastLightingPresetPath());
                m_modelPresetComboBox->SelectPath(viewportRequests->GetLastModelPresetPath());
                m_renderPipelineComboBox->SelectPath(viewportRequests->GetLastRenderPipelinePath());
            });
    }

    void EntityPreviewViewportToolBar::OnModelPresetAdded(const AZStd::string& path)
    {
        m_modelPresetComboBox->AddPath(path);
    }

    void EntityPreviewViewportToolBar::OnLightingPresetAdded(const AZStd::string& path)
    {
        m_lightingPresetComboBox->AddPath(path);
    }

    void EntityPreviewViewportToolBar::OnRenderPipelineAdded(const AZStd::string& path)
    {
        m_renderPipelineComboBox->AddPath(path);
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/EntityPreviewViewport/moc_EntityPreviewViewportToolBar.cpp>
