/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/stack.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/RoutingAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    //! Helper class that builds a DOM suitable for usage in a DocumentAdapter.
    //! Uses a visitor pattern to establish node elements.
    class AdapterBuilder
    {
    public:
        //! Creates an AdapterBuilder.
        //! A routing adapter may optionally be specified to enable routing via calls to
        //! NestedAdapter.
        //! Constructing an AdapterBuilder immediately opens an Adapter tag.
        AdapterBuilder(RoutingAdapter* routingAdapter = nullptr);

        AdapterBuilder& BeginRow();
        AdapterBuilder& EndRow();
        AdapterBuilder& BeginLabel(AZStd::string_view labelText, bool copy);
        AdapterBuilder& EndLabel();
        AdapterBuilder& BeginPropertyEditor(Name editorType);
        AdapterBuilder& EndPropertyEditor();
        //! Sets the value of the last node. Used for setting the current value of a property editor.
        AdapterBuilder& Value(Dom::Value value);
        //! Sets an attribute of the last node. Rows, labels, and property editors all support different attributes.
        //! \see DocumentPropertyEditor::Nodes
        AdapterBuilder& Attribute(Name attribute, Dom::Value value);
        //! Inserts a nested adapter. The contents of adapter will be inserted at the current path and kept up-to-date.
        //! Requires that this AdapterBuilder was constructed using a RoutingAdapter.
        AdapterBuilder& NestedAdapter(DocumentAdapterPtr adapter);

        //! Ends the build operation and retrieves the builder result.
        //! Operations are no longer valid on this builder once this is called.
        Dom::Value&& FinishAndTakeResult();

    private:
        Dom::Value& CurrentNode();
        void StartNode(AZ::Name name);
        void EndNode(AZ::Name expectedName = Name());

        RoutingAdapter* m_routingAdapter = nullptr;
        Dom::Value m_value;
        Dom::Path m_currentPath;
        AZStd::stack<Dom::Value> m_entries;
    };
} // namespace AZ::DocumentPropertyEditor
