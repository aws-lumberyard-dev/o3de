/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomValueWriter.h>
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

        // Node API
        class NodeScope
        {
        public:
            NodeScope(AdapterBuilder* adapter);
            NodeScope(const NodeScope&) = delete;
            NodeScope(NodeScope&& other);
            ~NodeScope();

            NodeScope& operator=(const NodeScope&) = delete;
            NodeScope& operator=(NodeScope&& other);

            NodeScope Row();
            NodeScope Label(AZStd::string_view labelText, bool copy);
            NodeScope PropertyEditor(Name editorType);
            NodeScope& Attribute(Name attribute, Dom::Value value);
            NodeScope& NestedAdapter(DocumentAdapterPtr adapter);

        private:
            AdapterBuilder* m_adapter;
        };
        NodeScope GetNodeBuilder();

        Dom::Value&& TakeValue();

    private:
        void StartNode();
        void EndNode();

        DocumentAdapter& m_adapter;
        Dom::Value m_value;
        Dom::ValueWriter m_writer;
        Dom::Path m_currentPath;
    };
} // namespace AZ::DocumentPropertyEditor
