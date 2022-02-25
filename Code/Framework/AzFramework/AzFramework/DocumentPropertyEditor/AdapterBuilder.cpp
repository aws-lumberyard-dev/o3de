/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>

namespace AZ::DocumentPropertyEditor
{
    AdapterBuilder::AdapterBuilder(DocumentAdapter& adapter)
        : m_adapter(adapter)
        , m_writer(m_value)
    {
        StartNode();
        m_writer.StartNode(AZ_NAME_LITERAL("Adapter"));
    }

    AdapterBuilder& AdapterBuilder::BeginRow()
    {
        StartNode();
        m_writer.StartNode(AZ_NAME_LITERAL("Row"));
        return *this;
    }

    AdapterBuilder& AdapterBuilder::EndRow()
    {
        EndNode();
        return *this;
    }

    AdapterBuilder& AdapterBuilder::BeginLabel(AZStd::string_view labelText, bool copy)
    {
        StartNode();
        m_writer.StartNode(AZ_NAME_LITERAL("Label"));
        m_writer.CurrentValue().SetNodeValue(Dom::Value(labelText, copy));
        return *this;
    }

    AdapterBuilder& AdapterBuilder::EndLabel()
    {
        EndNode();
        return *this;
    }

    AdapterBuilder& AdapterBuilder::BeginPropertyEditor(Name editorType)
    {
        StartNode();
        m_writer.StartNode(AZ_NAME_LITERAL("PropertyEditor"));
        return *this;
    }

    AdapterBuilder& AdapterBuilder::EndPropertyEditor()
    {
        EndNode();
        return *this;
    }

    AdapterBuilder& AdapterBuilder::Attribute(Name attribute, Dom::Value value)
    {
        m_writer.Key(attribute);
        value.Accept(m_writer, true);
        return *this;
    }

    AdapterBuilder& AdapterBuilder::NestedAdapter(DocumentAdapterPtr adapter)
    {
        AZ_Assert(m_adapter.SupportsRouting(), "DPE: Attempted to add a nested adapter to an adapter that doesn't support routing");
        m_adapter.GetRoutingAdapter()->AddRoute(m_currentPath, adapter);
        BeginRow();
        m_writer.CurrentValue() = adapter->GetContents();
        EndRow();
        return *this;
    }

    auto AdapterBuilder::GetNodeBuilder() -> NodeScope
    {
        return NodeScope(this);
    }

    Dom::Value&& AdapterBuilder::TakeValue()
    {
        EndRow();
        return AZStd::move(m_value);
    }

    void AdapterBuilder::StartNode()
    {
        const Dom::Value& currentValue = m_writer.CurrentValue();
        if (currentValue.IsNode())
        {
            m_currentPath.Push(currentValue.ArraySize());
        }
        else
        {
            m_currentPath.Push(0);
        }
    }

    void AdapterBuilder::EndNode()
    {
        m_writer.EndNode(m_writer.CurrentValue().MemberCount(), m_writer.CurrentValue().ArraySize());
    }

    AdapterBuilder::NodeScope::NodeScope(AdapterBuilder* adapter)
        : m_adapter(adapter)
    {
    }

    AdapterBuilder::NodeScope::NodeScope(NodeScope&& other)
        : m_adapter(other.m_adapter)
    {
        other.m_adapter = nullptr;
    }

    AdapterBuilder::NodeScope::~NodeScope()
    {
        if (m_adapter != nullptr)
        {
            m_adapter->EndNode();
        }
    }

    auto AdapterBuilder::NodeScope::operator=(NodeScope&& other) -> NodeScope&
    {
        m_adapter = other.m_adapter;
        other.m_adapter = nullptr;
        return *this;
    }

    auto AdapterBuilder::NodeScope::Row() -> NodeScope
    {
        m_adapter->BeginRow();
        return NodeScope(m_adapter);
    }

    auto AdapterBuilder::NodeScope::Label(AZStd::string_view labelText, bool copy) -> NodeScope
    {
        m_adapter->BeginLabel(labelText, copy);
        return NodeScope(m_adapter);
    }

    auto AdapterBuilder::NodeScope::PropertyEditor(Name editorType) -> NodeScope
    {
        m_adapter->BeginPropertyEditor(editorType);
        return NodeScope(m_adapter);
    }

    auto AdapterBuilder::NodeScope::Attribute(Name attribute, Dom::Value value) -> NodeScope&
    {
        m_adapter->Attribute(attribute, value);
        return *this;
    }

    auto AdapterBuilder::NodeScope::NestedAdapter(DocumentAdapterPtr adapter) -> NodeScope&
    {
        m_adapter->NestedAdapter(adapter);
        return *this;
    }
} // namespace AZ::DocumentPropertyEditor
