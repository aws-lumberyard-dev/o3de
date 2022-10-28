/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/DocumentPropertyEditor/ValueStringFilter.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>

#include <QObject>

namespace AZ::DocumentPropertyEditor
{
    class OutlinerFilterAdapter :
        public QObject,
        public ValueStringFilter
    {
        Q_OBJECT
    public:
        enum class GlobalSearchCriteria
        {
            Unlocked,
            Locked,
            Visible,
            Hidden,
            Separator,
            FirstRealFilter
        };

        OutlinerFilterAdapter();

        void SetFilterString(AZStd::string filterString);
        void SetCriteriaFilter(const AzQtComponents::SearchTypeFilterList& filterCriteria);

        struct EntityMatchNode : public ValueStringFilter::StringMatchNode
        {
            bool m_locked = false;
            bool m_visible = false;
        };

    public slots:
        void OnTextFilterChanged(const QString& filterText);

    protected:
        MatchInfoNode* NewMatchInfoNode() const override;
        void CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const override;
        bool MatchesFilter(MatchInfoNode* matchNode) const override;

        AzQtComponents::SearchTypeFilterList m_criteriaFilterList;
    };
} // namespace AZ::DocumentPropertyEditor
