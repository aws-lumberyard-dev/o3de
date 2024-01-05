/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/AssetSelection/AssetSelectionComboBox.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsNotificationBus.h>

#include <QAction>
#include <QToolBar>
#endif

namespace AtomToolsFramework
{
    //! EntityPreviewViewportToolBar contains common, easily accessible controls for manipulating viewport settings
    class EntityPreviewViewportToolBar final
        : public QToolBar
        , public EntityPreviewViewportSettingsNotificationBus::Handler
    {
        Q_OBJECT
    public:
        EntityPreviewViewportToolBar(const AZ::Crc32& toolId, QWidget* parent = 0);
        ~EntityPreviewViewportToolBar();

    private:
        // EntityPreviewViewportSettingsNotificationBus::Handler overrides...
        void OnViewportSettingsChanged() override;

        QAction* CreateThumbnailWidgetAction(
            QWidget* parent,
            const AZStd::string& title,
            const AZStd::string& path,
            bool checkable,
            bool checked,
            const AZStd::function<void()>& handler) const;
        QMenu* CreateToneMappingMenu() const;
        QMenu* CreateRenderPipelineMenu() const;
        QMenu* CreateLightingPresetMenu() const;
        QMenu* CreateModelPresetMenu() const;

        const AZ::Crc32 m_toolId{};
        QAction* m_showGrid{};
        QAction* m_showShadowCatcher{};
        QAction* m_showAlternateSkybox{};
    };
} // namespace AtomToolsFramework
