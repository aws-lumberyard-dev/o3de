/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/XML/XmlSerializationUtils.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/std/containers/stack.h>

namespace AZ::Dom::Xml
{
    Visitor::Result VisitSerializedXml(AZStd::string_view buffer, Visitor& visitor)
    {
        AZStd::string tempBuffer = buffer;
        return VisitSerializedXmlInPlace(tempBuffer.data(), Lifetime::Temporary, visitor);
    }

    Visitor::Result VisitXmlNode(const rapidxml::xml_node<>& node, Lifetime lifetime, Visitor& visitor)
    {
        Visitor::Result result = AZ::Success();
        AZ::u64 attributeCount = 0;
        AZ::u64 elementCount = 0;

        if (node.type() == rapidxml::node_type::node_data)
        {
            AZStd::string value(node.value(), node.value_size());
            Visitor::ResultCombine(result, visitor.RawValue(value, lifetime));
        }
        else if (node.type() == rapidxml::node_type::node_element)
        {
            AZStd::string_view name(node.name(), node.name_size());
            Visitor::ResultCombine(result, visitor.RawStartNode(name, lifetime));

            const rapidxml::xml_attribute<>* attribute = node.first_attribute();
            while (attribute != nullptr)
            {
                ++attributeCount;
                AZStd::string_view nameStr(attribute->name(), attribute->name_size());
                AZStd::string_view valueStr(attribute->value(), attribute->value_size());
                Visitor::ResultCombine(result, visitor.RawKey(nameStr, lifetime));
                Visitor::ResultCombine(result, visitor.RawValue(valueStr, lifetime));
                attribute = attribute->next_attribute();
            }
        }

        if (node.type() == rapidxml::node_type::node_document || node.type() == rapidxml::node_type::node_element)
        {
            const rapidxml::xml_node<>* childNode = node.first_node();
            while (childNode != nullptr)
            {
                ++elementCount;
                VisitXmlNode(*childNode, lifetime, visitor);
                childNode = childNode->next_sibling();
            }
        }

        if (node.type() == rapidxml::node_type::node_element)
        {
            Visitor::ResultCombine(result, visitor.EndNode(attributeCount, elementCount));
        }
        return result;
    }

    Visitor::Result VisitSerializedXmlInPlace(char* buffer, Lifetime lifetime, Visitor& visitor)
    {
        rapidxml::xml_document<> doc;
        doc.parse<0>(buffer);
        return VisitXmlNode(doc, lifetime, visitor);
    }

    class StreamWriter : public Visitor
    {
    public:
        StreamWriter(AZStd::string& buffer)
            : m_buffer(buffer)
        {
            m_stack.push({ &m_document });
        }

        ~StreamWriter()
        {
            m_stack.pop();
            rapidxml::print(AZStd::back_inserter(m_buffer), m_document);
        }

        VisitorFlags GetVisitorFlags() const override
        {
            return VisitorFlags::SupportsNodes | VisitorFlags::SupportsRawKeys | VisitorFlags::SupportsRawValues;
        }

        Result Null() override
        {
            return PushValue("null", Lifetime::Persistent);
        }

        Result Bool(bool value) override
        {
            return PushValue(value ? "true" : "false", Lifetime::Persistent);
        }

        Result Int64(AZ::s64 value) override
        {
            m_tempString = m_tempString.format("%" PRId64, value);
            return PushValue(m_tempString);
        }

        Result Uint64(AZ::u64 value) override
        {
            m_tempString = m_tempString.format("%" PRIu64, value);
            return PushValue(m_tempString);
        }

        Result Double(double value) override
        {
            m_tempString = m_tempString.format("%lf", value);
            return PushValue(m_tempString);
        }

        Result String(AZStd::string_view value, Lifetime lifetime) override
        {
            return PushValue(value, lifetime);
        }

        Result RawValue(AZStd::string_view value, Lifetime lifetime) override
        {
            return PushValue(value, lifetime);
        }

        Result RawKey(AZStd::string_view key, [[maybe_unused]] Lifetime lifetime) override
        {
            m_stack.top().m_key = key;
            return VisitorSuccess();
        }

        Result Key(AZ::Name key) override
        {
            return RawKey(key.GetStringView(), Lifetime::Temporary);
        }

        Result StartNode(AZ::Name name) override
        {
            return RawStartNode(name.GetStringView(), Lifetime::Temporary);
        }

        Result RawStartNode(AZStd::string_view name, [[maybe_unused]] Lifetime lifetime) override
        {
            m_stack.push({});
            StackEntry& top = m_stack.top();
            top.m_node = m_document.allocate_node(rapidxml::node_type::node_element, name.data(), 0, name.size(), 0);
            return VisitorSuccess();
        }

        Result EndNode(AZ::u64 attributeCount, AZ::u64 elementCount) override
        {
            StackEntry& top = m_stack.top();
            Result result = VisitorSuccess();
            if (attributeCount != top.m_attributeCount)
            {
                ResultCombine(
                    result,
                    VisitorFailure(
                        VisitorErrorCode::InvalidData,
                        AZStd::string::format("Expected %ull attributes, recieved %ull attributes", attributeCount, top.m_attributeCount)));
            }
            if (elementCount != top.m_elementCount)
            {
                ResultCombine(
                    result,
                    VisitorFailure(
                        VisitorErrorCode::InvalidData,
                        AZStd::string::format("Expected %ull elements, recieved %ull elements", elementCount, top.m_elementCount)));
            }
            rapidxml::xml_node<>* nodeToAdd = top.m_node;
            m_stack.pop();
            m_stack.top().m_node->append_node(nodeToAdd);
            return result;
        }

    private:
        Result PushValue(AZStd::string_view value, Lifetime lifetime = Lifetime::Temporary)
        {
            if (lifetime == Lifetime::Temporary)
            {
                value = CopyStringToDocument(value);
            }
            if (IsNextValueAttribute())
            {
                ++m_stack.top().m_attributeCount;
                GetCurrentNode().append_attribute(MakeAttribute(PopAttributeKey(), value));
            }
            else
            {
                ++m_stack.top().m_elementCount;
                GetCurrentNode().append_node(MakeValueNode(value, lifetime));
            }
            return Visitor::VisitorSuccess();
        }

        AZStd::string_view CopyStringToDocument(AZStd::string_view stringToCopy)
        {
            return AZStd::string_view(m_document.allocate_string(stringToCopy.data(), stringToCopy.size()), stringToCopy.size());
        }

        rapidxml::xml_attribute<>* MakeAttribute(AZStd::string_view name, AZStd::string_view value)
        {
            return m_document.allocate_attribute(name.data(), value.data(), name.size(), value.size());
        }

        rapidxml::xml_node<>* MakeValueNode(AZStd::string_view value, Lifetime lifetime)
        {
            if (lifetime == Lifetime::Temporary)
            {
                value = CopyStringToDocument(value);
            }
            return m_document.allocate_node(rapidxml::node_type::node_data, nullptr, value.data(), 0, value.size());
        }

        rapidxml::xml_node<>& GetCurrentNode()
        {
            return *m_stack.top().m_node;
        }

        bool IsNextValueAttribute() const
        {
            return !m_stack.top().m_key.empty();
        }

        AZStd::string_view PopAttributeKey()
        {
            AZStd::string_view key = m_stack.top().m_key;
            m_stack.top().m_key = {};
            return key;
        }

        struct StackEntry
        {
            rapidxml::xml_node<>* m_node;
            AZStd::string_view m_key;
            AZ::u64 m_attributeCount = 0;
            AZ::u64 m_elementCount = 0;
        };
        AZStd::stack<StackEntry> m_stack;
        rapidxml::xml_document<> m_document;
        AZStd::string& m_buffer;
        // Used as a temporary buffer for formatting numeric types
        AZStd::fixed_string<64> m_tempString;
    };

    AZStd::unique_ptr<Visitor> CreateXmlBufferWriter(AZStd::string& buffer)
    {
        return AZStd::make_unique<StreamWriter>(buffer);
    }
} // namespace AZ::Dom::Xml
