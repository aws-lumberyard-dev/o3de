/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/YAML/YamlUtils.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>

namespace AZ::Dom::Yaml
{
    struct Parser
    {
        Parser(Visitor& visitor, Lifetime lifetime)
            : m_visitor(visitor)
            , m_lifetime(lifetime)
        {
            m_truePattern = AZStd::regex("^([Yy][Ee]?[Ss]?)|([Tt][Rr][Uu][Ee])");
            m_falsePattern = AZStd::regex("^([Nn][Oo]?)|([Ff][Aa][Ll][Ss][Ee])");
            m_nullPattern = AZStd::regex("^(null)|(~)$");
        }

        Visitor::Result ParseValue(ryml::NodeRef node)
        {
            AZStd::string_view value(node.val().data(), node.val().size());

            AZStd::match_results<const char*> match;
            if (value.empty() || AZStd::regex_search(value.data(), match, m_nullPattern))
            {
                return m_visitor.Null();
            }

            if (m_visitor.SupportsRawValues())
            {
                return m_visitor.RawValue(value, m_lifetime);
            }

            if (AZStd::regex_search(value.data(), match, m_truePattern))
            {
                return m_visitor.Bool(true);
            }
            if (AZStd::regex_search(value.data(), match, m_falsePattern))
            {
                return m_visitor.Bool(false);
            }

            const c4::csubstr& nodeVal = node.val();
            if (nodeVal.is_unsigned_integer())
            {
                return m_visitor.Uint64(strtoull(value.data(), nullptr, 0));
            }
            else if (nodeVal.is_unsigned_integer())
            {
                return m_visitor.Int64(strtoll(value.data(), nullptr, 0));
            }
            else
            {
                return m_visitor.Double(strtod(value.data(), nullptr));
            }
        }

        Visitor::Result VisitNode(ryml::NodeRef node)
        {
            if (node.has_key())
            {
                Visitor::Result result = m_visitor.RawKey({ node.key().data(), node.key().size() }, m_lifetime);
                if (!result.IsSuccess())
                {
                    return result;
                }
            }

            if (node.has_val())
            {
                return ParseValue(node);
            }

            if (node.is_map())
            {
                size_t childCount = 0;
                m_visitor.StartObject();
                for (ryml::NodeRef child : node.children())
                {
                    VisitNode(child);
                    ++childCount;
                }
                return m_visitor.EndObject(childCount);
            }

            if (node.is_seq())
            {
                size_t childCount = 0;
                m_visitor.StartArray();
                for (ryml::NodeRef child : node.children())
                {
                    VisitNode(child);
                    ++childCount;
                }
                return m_visitor.EndArray(childCount);
            }

            return AZ::Failure<VisitorError>(
                VisitorError(VisitorErrorCode::UnsupportedOperation, "AZ::Dom::Value::VisitNode: unsupported YAML type"));
        }

        Visitor& m_visitor;
        Lifetime m_lifetime;

        AZStd::regex m_truePattern;
        AZStd::regex m_falsePattern;
        AZStd::regex m_nullPattern;
    };

    class StreamWriter final : public Visitor
    {
    public:
        StreamWriter(AZStd::string& buffer)
            : m_buffer(buffer)
        {
        }

        ~StreamWriter()
        {
            ryml::emitrs(m_tree, &m_buffer);
        }

        VisitorFlags GetVisitorFlags() const override
        {
            return VisitorFlags::SupportsRawKeys | VisitorFlags::SupportsArrays | VisitorFlags::SupportsObjects;
        }

        Result Null() override
        {
            CurrentNode() << "~";
            return VisitorSuccess();
        }

        Result Bool(bool value) override
        {
            CurrentNode() << (value ? "true" : "false");
            return VisitorSuccess();
        }

        Result Int64(int64_t value) override
        {
            CurrentNode() << value;
            return VisitorSuccess();
        }

        Result Uint64(uint64_t value) override
        {
            CurrentNode() << value;
            return VisitorSuccess();
        }

        Result Double(double value) override
        {
            CurrentNode() << value;
            return VisitorSuccess();
        }

        Result String(AZStd::string_view value, Lifetime lifetime)
        {
            ryml::NodeRef entry = CurrentNode();
            entry |= ryml::VALQUO ;
            c4::csubstr valueStr(value.data(), value.size());
            if (lifetime == Lifetime::Persistent)
            {
                entry = valueStr;
            }
            else
            {
                entry << valueStr;
            }
            return VisitorSuccess();
        }

        Result StartObject() override
        {
            ryml::NodeRef entry = CurrentNode();
            entry |= ryml::MAP;
            m_stack.push({ entry });
            return VisitorSuccess();
        }

        Result EndObject(AZ::u64 attributeCount) override
        {
            if (m_stack.top().m_node.num_children() != attributeCount)
            {
                return VisitorFailure(VisitorErrorCode::InvalidData, "EndObject failure");
            }
            m_stack.pop();
            return VisitorSuccess();
        }

        Result Key(AZ::Name key) override
        {
            return RawKey(key.GetStringView(), Lifetime::Persistent);
        }

        Result RawKey(AZStd::string_view key, Lifetime lifetime) override
        {
            ryml::NodeRef node = CurrentNode();
            m_stack.top().m_next = node;
            c4::csubstr keyStr(key.data(), key.size());
            if (lifetime == Lifetime::Persistent)
            {
                node.set_key_ref(keyStr);
            }
            else
            {
                node << ryml::key(keyStr);
            }
            return VisitorSuccess();
        }

        Result StartArray() override
        {
            ryml::NodeRef entry = CurrentNode();
            entry |= ryml::SEQ;
            m_stack.push({ entry });
            return VisitorSuccess();
        }

        Result EndArray(AZ::u64 elementCount) override
        {
            if (m_stack.top().m_node.num_children() != elementCount)
            {
                return VisitorFailure(VisitorErrorCode::InvalidData, "EndArray failure");
            }
            m_stack.pop();
            return VisitorSuccess();
        }

    private:
        ryml::NodeRef CurrentNode()
        {
            if (!m_stack.empty())
            {
                Entry& top = m_stack.top();
                if (top.m_next.has_value())
                {
                    ryml::NodeRef next = top.m_next.value();
                    top.m_next.reset();
                    return next;
                }
                return top.m_node.append_child();
            }
            return m_tree.rootref();
        }

        AZStd::string& m_buffer;
        ryml::Tree m_tree;

        struct Entry
        {
            ryml::NodeRef m_node;
            AZStd::optional<ryml::NodeRef> m_next;
        };
        AZStd::stack<Entry> m_stack;
    };

    Visitor::Result VisitSerializedYaml(AZStd::string_view buffer, Lifetime lifetime, Visitor& visitor)
    {
        ryml::Tree tree = ryml::parse(c4::csubstr(buffer.data(), buffer.size()));
        Parser parser(visitor, lifetime);
        return parser.VisitNode(tree.rootref());
    }

    AZStd::unique_ptr<Visitor> CreateYamlBufferWriter(AZStd::string& buffer)
    {
        return AZStd::make_unique<StreamWriter>(buffer);
    }
} // namespace AZ::Dom::Yaml
