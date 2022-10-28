/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/DocumentPropertyEditor/FilterAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    class OutlinerFilterAdapter : public ValueStringFilter
    {
    public:
        OutlinerFilterAdapter();

        void SetFilterString(AZStd::string filterString);
        void SetCriteriaFilter(SearchTypeFilterList filterCriteria);

        struct EntityMatchNode : public ValueStringFilter::StringMatchNode
        {
            bool m_locked = false;
            bool m_visible = false;
        };

    protected:
        MatchInfoNode* NewMatchInfoNode() const override;
        void CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const override;
        bool MatchesFilter(MatchInfoNode* matchNode) const override;
    };
} // namespace AZ::DocumentPropertyEditor
