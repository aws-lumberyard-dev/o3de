/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/Nodes.h>

namespace AZ::DocumentPropertyEditor
{
    AdapterBuilder::AdapterBuilder(RoutingAdapter* routingAdapter)
        : m_routingAdapter(routingAdapter)
    {
        StartNode(Nodes::Adapter::Name);
    }

    AdapterBuilder& AdapterBuilder::BeginRow()
    {
        StartNode(Nodes::Row::Name);
        return *this;
    }

    AdapterBuilder& AdapterBuilder::EndRow()
    {
        EndNode(Nodes::Row::Name);
        return *this;
    }

    AdapterBuilder& AdapterBuilder::BeginLabel(AZStd::string_view labelText, bool copy)
    {
        StartNode(Nodes::Label::Name);
        CurrentNode().SetNodeValue(Dom::Value(labelText, copy));
        return *this;
    }

    AdapterBuilder& AdapterBuilder::EndLabel()
    {
        EndNode(Nodes::Label::Name);
        return *this;
    }

    AdapterBuilder& AdapterBuilder::BeginPropertyEditor(Name editorType)
    {
        StartNode(Nodes::PropertyEditor::Name);
        Attribute(Nodes::PropertyEditor::Type, Dom::Value(editorType.GetStringView(), true));
        return *this;
    }

    AdapterBuilder& AdapterBuilder::EndPropertyEditor()
    {
        EndNode();
        return *this;
    }

    AdapterBuilder& AdapterBuilder::Value(Dom::Value value)
    {
        CurrentNode().SetNodeValue(value);
        return *this;
    }

    AdapterBuilder& AdapterBuilder::Attribute(Name attribute, Dom::Value value)
    {
        CurrentNode()[attribute] = AZStd::move(value);
        return *this;
    }

    AdapterBuilder& AdapterBuilder::NestedAdapter(DocumentAdapterPtr adapter)
    {
        AZ_Assert(m_routingAdapter, "DPE: Attempted to add a nested adapter to a builder with no registered routing adapter");
        m_routingAdapter->AddRoute(m_currentPath, adapter);
        BeginRow();
        CurrentNode() = adapter->GetContents();
        EndRow();
        return *this;
    }

    Dom::Value&& AdapterBuilder::FinishAndTakeResult()
    {
        EndNode(Nodes::Adapter::Name);
        return AZStd::move(m_value);
    }

    Dom::Value& AdapterBuilder::CurrentNode()
    {
        AZ_Assert(!m_entries.empty(), "AdapterBuilder::CurrentNode called without a node on the entry stack");
        return m_entries.top();
    }

    void AdapterBuilder::StartNode(Name name)
    {
        if (!m_entries.empty())
        {
            m_currentPath.Push(CurrentNode().ArraySize());
        }
        else
        {
            m_currentPath.Push(0);
        }
        m_entries.push(Dom::Value::CreateNode(name));
    }

    void AdapterBuilder::EndNode([[maybe_unused]] AZ::Name expectedName)
    {
        AZ_Assert(!m_entries.empty(), "AdapterBuilder::EndNode called with an empty entry stack");
        AZ_Assert(
            expectedName.IsEmpty() || CurrentNode().GetNodeName() == expectedName,
            "AdapterBuilder::EndNode called for %s when %s was expected", CurrentNode().GetNodeName().GetCStr(), expectedName.GetCStr());
        m_currentPath.Pop();
        Dom::Value value = AZStd::move(m_entries.top());
        m_entries.pop();
        if (!m_entries.empty())
        {
            CurrentNode().ArrayPushBack(AZStd::move(value));
        }
        else
        {
            m_value = AZStd::move(value);
        }
    }
} // namespace AZ::DocumentPropertyEditor
