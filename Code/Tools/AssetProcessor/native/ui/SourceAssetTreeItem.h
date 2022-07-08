/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <ui/AssetTreeItem.h>

namespace AssetProcessor
{
    enum class SourceAssetTreeColumns
    {
        AnalysisJobDuration = aznumeric_cast<int>(AssetTreeColumns::Max),
        Max
    };

    class SourceAssetTreeItem : public AssetTreeItem
    {
        explicit SourceAssetTreeItem(
            AZStd::shared_ptr<AssetTreeItemData> data,
            QIcon errorIcon,
            QIcon folderIcon,
            QIcon fileIcon,
            AssetTreeItem* parentItem = nullptr);
        virtual ~SourceAssetTreeItem();

        virtual int GetColumnCount() const override;

        // Overriding AssetTreeModel for displaying analysis job duration
        virtual QVariant GetDataForColumn(int column) const;
    };
}
