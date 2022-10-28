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
                if (m_filterString.empty())
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

    RowFilterAdapter::MatchInfoNode* OutlinerFilterAdapter::NewMatchInfoNode() const
    {
        return new EntityMatchNode();
    }

    void OutlinerFilterAdapter::CacheDomInfoForNode(const Dom::Value& domValue, MatchInfoNode* matchNode) const
    {
        auto entityMatchNode = static_cast<EntityMatchNode*>(matchNode);
        const bool nodeIsRow = IsRow(domValue);
        AZ_Assert(nodeIsRow, "Only row nodes should be cached by a RowFilterAdapter");
        if (nodeIsRow)
        {
            entityMatchNode->m_matchableDomTerms.clear();
            for (auto childIter = domValue.ArrayBegin(); childIter != domValue.ArrayEnd(); ++childIter)
            {
                if (childIter->IsNode())
                {
                    auto childName = childIter->GetNodeName();
                    if (childName != Dpe::GetNodeName<Nodes::Row>()) // don't cache child rows, they have their own entries
                    {
                        if (auto foundValue = childIter->FindMember(Nodes::OutlinerRow::Value.GetName());
                            foundValue != childIter->MemberEnd())
                        {
                            entityMatchNode->AddStringifyValue(foundValue->second);
                        }

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

                        if (m_includeDescriptions)
                        {
                            auto foundDescription = childIter->FindMember(Nodes::PropertyEditor::Description.GetName());
                            if (foundDescription != childIter->MemberEnd())
                            {
                                entityMatchNode->AddStringifyValue(foundDescription->second);
                            }
                        }
                    }
                }
            }
        }
    }

    bool OutlinerFilterAdapter::MatchesFilter(MatchInfoNode* matchNode) const
    {
        auto entityMatchNode = static_cast<StringMatchNode*>(matchNode);
        return (m_filterString.empty() || entityMatchNode->m_matchableDomTerms.contains(m_filterString));
    }
} // namespace AZ::DocumentPropertyEditor
