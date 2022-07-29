/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QPointer>
#endif

namespace AssetProcessor
{
    class BuilderData;

    class BuilderListModel : QAbstractItemModel
    {
        Q_OBJECT;

    public:
        enum class Column
        {
            Name,
            JobCount,
            TotalDuration,
            AverageDuration,
            Max
        };

        BuilderListModel(QPointer<BuilderData> builderData, QObject* parent = nullptr)
            : QAbstractItemModel(parent), m_data(builderData)
        {
        }
        void Reset();

        // QAbstractItemModel
        QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
        int rowCount(const QModelIndex& parent) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        QModelIndex parent(const QModelIndex& index) const override;

    private:
        QPointer<BuilderData> m_data;
    };
}
