/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/UI/DocumentPropertyEditor/OutlinerFilterAdapter.h>
#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

namespace AZ::DocumentPropertyEditor
{
    OutlinerFilterAdapter::OutlinerFilterAdapter()
        : ValueStringFilter()
    {
    }

    void OutlinerFilterAdapter::SetFilterString(AZStd::string filterString)
    {
        AZStd::to_lower(filterString.begin(), filterString.end());
        if (m_filterString != filterString)
        {
            m_filterString = AZStd::move(filterString);

            if (m_filterActive)
            {
                if (m_filterString.empty() && m_criteriaFilterList.empty())
                {
                    SetFilterActive(false);
                }
                else
                {
                    InvalidateFilter();
                }
            }
            else if (!m_filterString.empty())
            {
                SetFilterActive(true);
            }
        }
    }

    void OutlinerFilterAdapter::SetCriteriaFilter(const AzQtComponents::SearchTypeFilterList& filterCriteria)
    {
        m_criteriaFilterList = filterCriteria;

        if (!m_criteriaFilterList.empty())
        {
            SetFilterActive(true);
            InvalidateFilter();
        }
        else if (m_filterString.empty())
        {
            SetFilterActive(false);
        }
    }

    void OutlinerFilterAdapter::OnTextFilterChanged(const QString& filterText)
    {
        SetFilterString(filterText.toUtf8().data());
    }

    RowFilterAdapter::MatchInfoNode* OutlinerFilterAdapter::NewMatchInfoNode() const
    {
        return new EntityMatchNode();
    }

    void OutlinerFilterAdapter::CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const
    {
        ValueStringFilter::CacheDomInfoForNode(domValue, matchNode);

        auto entityMatchNode = static_cast<EntityMatchNode*>(matchNode);
        const bool nodeIsRow = IsRow(domValue);
        AZ_Assert(nodeIsRow, "Only row nodes should be cached by a RowFilterAdapter");
        if (nodeIsRow)
        {
            for (auto childIter = domValue.ArrayBegin(); childIter != domValue.ArrayEnd(); ++childIter)
            {
                if (childIter->IsNode())
                {
                    auto childName = childIter->GetNodeName();
                    if (childName != Dpe::GetNodeName<Nodes::Row>()) // don't cache child rows, they have their own entries
                    {
                        if (auto visibleValue = childIter->FindMember(Nodes::OutlinerRow::Visible.GetName());
                            visibleValue != childIter->MemberEnd())
                        {
                            entityMatchNode->m_visible = visibleValue->second.GetBool();
                        }

                        if (auto lockedValue = childIter->FindMember(Nodes::OutlinerRow::Locked.GetName());
                                 lockedValue != childIter->MemberEnd())
                        {
                            entityMatchNode->m_locked = lockedValue->second.GetBool();
                        }
                    }
                }
            }
        }
    }

    bool OutlinerFilterAdapter::MatchesFilter(MatchInfoNode* matchNode) const
    {
        auto entityMatchNode = static_cast<EntityMatchNode*>(matchNode);
        bool matches = ValueStringFilter::MatchesFilter(matchNode);

        if (matches)
        {
            bool matchesLocked = false;
            bool matchesUnLocked = false;
            bool matchesHidden = false;
            bool matchesVisible = false;

            for (auto& criterion : m_criteriaFilterList)
            {
                if (criterion.globalFilterValue >= 0)
                {
                    switch (criterion.globalFilterValue)
                    {
                    case 0: // GlobalSearchCriteriaFlags::Unlocked
                        matchesUnLocked = true;
                        break;
                    case 1: // GlobalSearchCriteriaFlags::Locked
                        matchesLocked = true;
                        break;
                    case 2: // GlobalSearchCriteriaFlags::Visible
                        matchesVisible = true;
                        break;
                    case 3: // GlobalSearchCriteriaFlags::Hidden
                        matchesHidden = true;
                        break;
                    }
                }
            }

            if (matchesLocked && !entityMatchNode->m_locked)
            {
                matches = false;
            }
            if (matchesUnLocked && entityMatchNode->m_locked)
            {
                matches = false;
            }
            if (matchesHidden && entityMatchNode->m_visible)
            {
                matches = false;
            }
            if (matchesVisible && !entityMatchNode->m_visible)
            {
                matches = false;
            }
        }

        return matches;
    }
} // namespace AZ::DocumentPropertyEditor
