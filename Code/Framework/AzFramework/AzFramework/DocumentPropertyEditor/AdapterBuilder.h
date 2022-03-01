/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/RoutingAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    class AdapterBuilder
    {
    public:
        AdapterBuilder(DocumentAdapter& adapter);

        // Visitor API
        AdapterBuilder& BeginRow();
        AdapterBuilder& EndRow();
        AdapterBuilder& BeginLabel(AZStd::string_view labelText, bool copy);
        AdapterBuilder& EndLabel();
        AdapterBuilder& BeginPropertyEditor(Name editorType);
        AdapterBuilder& EndPropertyEditor();
        AdapterBuilder& Attribute(Name attribute, Dom::Value value);
        AdapterBuilder& NestedAdapter(DocumentAdapterPtr adapter);

        Dom::Value&& TakeValue();

    private:
        Dom::Value& CurrentNode();
        void StartNode(AZ::Name name);
        void EndNode();

        DocumentAdapter& m_adapter;
        Dom::Value m_value;
        Dom::Path m_currentPath;
        AZStd::vector<Dom::Value> m_entries;
    };
} // namespace AZ::DocumentPropertyEditor
