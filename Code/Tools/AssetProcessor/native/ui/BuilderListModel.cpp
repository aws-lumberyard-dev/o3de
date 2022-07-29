/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <native/ui/BuilderListModel.h>
#include <native/ui/BuilderData.h>
#include <native/ui/BuilderDataItem.h>

namespace AssetProcessor
{
    QModelIndex BuilderListModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
        {
            return QModelIndex();
        }

        BuilderDataItem* const parentItem =
            parent.isValid() ? static_cast<BuilderDataItem*>(parent.internalPointer()) : m_data->m_root.get();

        // parentItem is the invisible root. The only child row is the "All Builders"
        if (parentItem == m_data->m_root.get() && row == 0)
        {
            QModelIndex index = createIndex(row, column, m_data->m_allBuildersMetrics.get());
            Q_ASSERT(checkIndex(index));
            return index;
        }
        // parentItem is "All Builders". All the builders are its child rows.
        else if (parentItem == m_data->m_allBuildersMetrics.get() && row < m_data->m_singleBuilderMetrics.size())
        {
            QModelIndex index = createIndex(row, column, m_data->m_singleBuilderMetrics[row].get());
            Q_ASSERT(checkIndex(index));
            return index;
        }

        return QModelIndex();
    }

    int BuilderListModel::rowCount(const QModelIndex& parent) const
    {
        BuilderDataItem* const parentItem =
            parent.isValid() ? static_cast<BuilderDataItem*>(parent.internalPointer()) : m_data->m_root.get();

        // parentItem is the invisible root. The only child is the "All Builders"
        if (parentItem == m_data->m_root.get())
        {
            return 1;
        }
        // parentItem is "All Builders". All the builders are its children.
        else if (parentItem == m_data->m_allBuildersMetrics.get())
        {
            return aznumeric_cast<int>(m_data->m_singleBuilderMetrics.size());
        }
        // parentItem is a builder or other unrecognized item.
        return 0;
    }

    int BuilderListModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return aznumeric_cast<int>(Column::Max);
    }

    QVariant BuilderListModel::data(const QModelIndex& index, int role) const
    {
        if (role != Qt::DisplayRole || !index.isValid())
        {
            return QVariant();
        }

        BuilderDataItem* item = static_cast<BuilderDataItem*>(index.internalPointer());
        switch (index.column())
        {
        case aznumeric_cast<int>(Column::Name):
            return item->GetName();
        case aznumeric_cast<int>(Column::JobCount):
            return item->GetJobCount();
        case aznumeric_cast<int>(Column::AverageDuration):
            if (item->GetJobCount() == 0)
            {
                return QVariant();
            }
            return BuilderDataItem::DurationToQString(item->GetTotalDuration() / item->GetJobCount());
        case aznumeric_cast<int>(Column::TotalDuration):
            return BuilderDataItem::DurationToQString(item->GetTotalDuration());
        default:
            break;
        }

        return QVariant();
    }

    QVariant BuilderListModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section >= aznumeric_cast<int>(Column::Max))
        {
            return QVariant();
        }

        switch (section)
        {
        case aznumeric_cast<int>(Column::Name):
            return tr("Name");
        case aznumeric_cast<int>(Column::JobCount):
            return tr("Job Count");
        case aznumeric_cast<int>(Column::AverageDuration):
            return tr("Average Duration");
        case aznumeric_cast<int>(Column::TotalDuration):
            return tr("Total Duration");
        default:
            AZ_Warning("Asset Processor", false, "Unhandled BuilderInfoMetricsModel header %d", section);
            break;
        }
        return QVariant();
    }

    QModelIndex BuilderListModel::parent(const QModelIndex& index) const
    {
        BuilderDataItem* const currentItem =
            index.isValid() ? static_cast<BuilderDataItem*>(index.internalPointer()) : m_data->m_root.get();

        // currentItem is the invisible root or nullptr. There is no parent.
        if (currentItem == m_data->m_root.get() || currentItem == nullptr)
        {
            return QModelIndex();
        }
        // currentItem is "All Builders". The parent is "invisible root".
        else if (currentItem == m_data->m_allBuildersMetrics.get())
        {
            QModelIndex parentIndex = createIndex(0, 0, m_data->m_root.get());
            Q_ASSERT(checkIndex(parentIndex));
            return parentIndex;
        }
        // anything left should be builders. Their parent is "All Builders".
        QModelIndex parentIndex = createIndex(0, 0, m_data->m_allBuildersMetrics.get());
        Q_ASSERT(checkIndex(parentIndex));
        return parentIndex;
    }

    void BuilderListModel::Reset()
    {
        beginResetModel();
        endResetModel();
    }
} // namespace AssetProcessor
