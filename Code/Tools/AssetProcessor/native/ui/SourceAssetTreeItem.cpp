/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ui/SourceAssetTreeItem.h>
#include <ui/SourceAssetTreeItemData.h>

#include <QVariant>

namespace AssetProcessor
{
    SourceAssetTreeItem::SourceAssetTreeItem(
        AZStd::shared_ptr<AssetTreeItemData> data, QIcon errorIcon, QIcon folderIcon, QIcon fileIcon, AssetTreeItem* parentItem)
        : AssetTreeItem(data, errorIcon, folderIcon, fileIcon, parentItem)
    {
    }

    SourceAssetTreeItem::~SourceAssetTreeItem()
    {
    }

    int SourceAssetTreeItem::GetColumnCount() const
    {
        return aznumeric_cast<int>(SourceAssetTreeColumns::Max);
    }

    QVariant SourceAssetTreeItem::GetDataForColumn(int column) const
    {
        if (column < 0 || column >= GetColumnCount() || !m_data)
        {
            return QVariant();
        }

        switch (column)
        {
            case aznumeric_cast<int>(SourceAssetTreeColumns::AnalysisJobDuration):
                return AZStd::rtti_pointer_cast<SourceAssetTreeItemData>(m_data)->m_analysisDuration;
            default:
                return AssetTreeItem::GetDataForColumn(column);
        }
    }
} // namespace AssetProcessor
