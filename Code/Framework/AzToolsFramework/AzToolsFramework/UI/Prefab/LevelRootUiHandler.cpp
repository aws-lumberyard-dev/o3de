/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/LevelRootUiHandler.h>

#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/UI/Outliner/EntityOutlinerListModel.hxx>

#include <QAbstractItemModel>
#include <QPainter>
#include <QPainterPath>
#include <QTreeView>

namespace AzToolsFramework
{
    const QColor LevelRootUiHandler::s_levelRootBorderColor = QColor("#656565");
    const QString LevelRootUiHandler::s_levelRootIconPath = QString(":/Level/level.svg");

    LevelRootUiHandler::LevelRootUiHandler()
    {
        m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
        if (m_prefabPublicInterface == nullptr)
        {
            AZ_Assert(false, "LevelRootUiHandler - could not get PrefabPublicInterface on LevelRootUiHandler construction.");
            return;
        }

        m_prefabFocusPublicInterface = AZ::Interface<Prefab::PrefabFocusPublicInterface>::Get();
        if (m_prefabFocusPublicInterface == nullptr)
        {
            AZ_Assert(false, "LevelRootUiHandler - could not get PrefabFocusPublicInterface on LevelRootUiHandler construction.");
            return;
        }
    }

    QIcon LevelRootUiHandler::GenerateItemIcon([[maybe_unused]] AZ::EntityId entityId) const
    {
        return QIcon(s_levelRootIconPath);
    }

    QString LevelRootUiHandler::GenerateItemInfoString(AZ::EntityId entityId) const
    {
        QString infoString;

        AZ::IO::Path path = m_prefabPublicInterface->GetOwningInstancePrefabPath(entityId);

        if (!path.empty())
        {
            QString saveFlag = "";
            auto dirtyOutcome = m_prefabPublicInterface->HasUnsavedChanges(path);

            if (dirtyOutcome.IsSuccess() && dirtyOutcome.GetValue() == true)
            {
                saveFlag = "*";
            }

            infoString = QObject::tr("<span style=\"font-style: italic; font-weight: 400;\">(%1%2)</span>")
                             .arg(path.Filename().Native().data())
                             .arg(saveFlag);
        }

        return infoString;
    }

    bool LevelRootUiHandler::CanToggleLockVisibility([[maybe_unused]] AZ::EntityId entityId) const
    {
        return false;
    }

    bool LevelRootUiHandler::CanRename([[maybe_unused]] AZ::EntityId entityId) const
    {
        return false;
    }

    void LevelRootUiHandler::PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, [[maybe_unused]] const QModelIndex& index) const
    {
        if (!painter)
        {
            AZ_Warning("LevelRootUiHandler", false, "LevelRootUiHandler - painter is nullptr, can't draw Prefab outliner background.");
            return;
        }

        const bool isFirstColumn = index.column() == EntityOutlinerListModel::ColumnName;
        const bool isLastColumn = index.column() == EntityOutlinerListModel::ColumnLockToggle;

        // QPen borderLinePen(s_levelRootBorderColor, s_levelRootBorderThickness);
        QRect rect = option.rect;

        QColor backgroundColor = QColor("#1E252F");
        AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();

        if (m_prefabFocusPublicInterface->IsOwningPrefabBeingFocused(levelContainerEntityId))
        {
            backgroundColor = QColor("#4A90E2");
        }
        else if (!(option.state & QStyle::State_Enabled))
        {
            backgroundColor = QColor("#35383C");
        }

        QPainterPath backgroundPath;
        backgroundPath.setFillRule(Qt::WindingFill);

        QRect tempRect = option.rect;
        // TOP & BOTTOM
        tempRect.setTop(tempRect.top() + 1);
        tempRect.setHeight(tempRect.height() - 1);

        if (isFirstColumn)
        {
            tempRect.setLeft(tempRect.left() + 1);
            tempRect.setWidth(tempRect.width() + 1); // remove the line
        }

        if (isLastColumn)
        {
            tempRect.setLeft(tempRect.left() - 1); // remove the line
            tempRect.setWidth(tempRect.width() - 1);
        }

        if (isFirstColumn || isLastColumn)
        {
            // Rounded rect to have rounded borders on top.
            backgroundPath.addRoundedRect(tempRect, 6, 6);

            // Regular rect, half height, to square the opposite border
            QRect squareRect = tempRect;
            if (isFirstColumn)
            {
                squareRect.setLeft(tempRect.left() + (tempRect.width() / 2));
            }
            else if (isLastColumn)
            {
                squareRect.setWidth(tempRect.width() / 2);
            }
            backgroundPath.addRect(squareRect);
        }
        else
        {
            backgroundPath.addRect(tempRect);
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Draw border at the bottom
        // painter->setPen(borderLinePen);
        // painter->drawLine(rect.bottomLeft(), rect.bottomRight());
        
        painter->fillPath(backgroundPath.simplified(), backgroundColor);

        painter->restore();
    }

    bool LevelRootUiHandler::OnOutlinerItemClick(
        [[maybe_unused]] const QPoint& position,
        [[maybe_unused]] const QStyleOptionViewItem& option,
        [[maybe_unused]] const QModelIndex& index) const
    {
        return false;
    }

    bool LevelRootUiHandler::OnOutlinerItemDoubleClick(const QModelIndex& index) const
    {
        AZ::EntityId entityId = GetEntityIdFromIndex(index);

        if (auto prefabFocusPublicInterface = AZ::Interface<Prefab::PrefabFocusPublicInterface>::Get();
            !prefabFocusPublicInterface->IsOwningPrefabBeingFocused(entityId))
        {
            prefabFocusPublicInterface->FocusOnOwningPrefab(entityId);
        }

        // Don't propagate event.
        return true;
    }
}
