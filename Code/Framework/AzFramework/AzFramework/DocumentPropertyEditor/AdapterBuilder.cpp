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
    AdapterBuilder::AdapterBuilder(DocumentAdapter& adapter)
        : m_adapter(adapter)
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
        EndNode();
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
        EndNode();
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

    AdapterBuilder& AdapterBuilder::Attribute(Name attribute, Dom::Value value)
    {
        CurrentNode()[attribute] = AZStd::move(value);
        return *this;
    }

    AdapterBuilder& AdapterBuilder::NestedAdapter(DocumentAdapterPtr adapter)
    {
        AZ_Assert(m_adapter.SupportsRouting(), "DPE: Attempted to add a nested adapter to an adapter that doesn't support routing");
        m_adapter.GetRoutingAdapter()->AddRoute(m_currentPath, adapter);
        BeginRow();
        CurrentNode() = adapter->GetContents();
        EndRow();
        return *this;
    }

    Dom::Value&& AdapterBuilder::TakeValue()
    {
        EndRow();
        return AZStd::move(m_value);
    }

    Dom::Value& AdapterBuilder::CurrentNode()
    {
        AZ_Assert(!m_entries.empty(), "AdapterBuilder::CurrentNode called without a node on the entry stack");
        return m_entries[m_entries.size() - 1];
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
        m_entries.push_back(Dom::Value::CreateNode(name));
    }

    void AdapterBuilder::EndNode()
    {
        AZ_Assert(!m_entries.empty(), "AdapterBuilder::EndNode called with an empty entry stack");
        m_currentPath.Pop();
        Dom::Value value = AZStd::move(m_entries[m_entries.size() - 1]);
        m_entries.pop_back();
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
