/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomVisitor.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/string/regex.h>

namespace AZ::Dom
{
    class DomPolyfillVisitor : public Visitor
    {
    public:
        explicit DomPolyfillVisitor(AZStd::unique_ptr<Visitor>&& visitorToProxy);
        explicit DomPolyfillVisitor(Visitor& visitorToProxy);

        template<typename VisitorToPolyfill, typename... Args>
        static AZStd::unique_ptr<DomPolyfillVisitor> CreatePolyfillProxy(Args... args)
        {
            return AZStd::make_unique<DomPolyfillVisitor>(AZStd::make_unique<VisitorToPolyfill>(args...));
        }

        VisitorFlags GetVisitorFlags() const override;
        Result Null() override;
        Result Bool(bool value) override;
        Result Int64(AZ::s64 value) override;
        Result Uint64(AZ::u64 value) override;
        Result Double(double value) override;
        Result String(AZStd::string_view value, Lifetime lifetime) override;
        Result RawValue(AZStd::string_view value, Lifetime lifetime) override;

        Result StartObject() override;
        Result EndObject(AZ::u64 attributeCount) override;
        Result Key(AZ::Name key) override;
        Result RawKey(AZStd::string_view key, Lifetime lifetime) override;
        Result StartArray() override;
        Result EndArray(AZ::u64 elementCount) override;
        Result StartNode(AZ::Name name) override;
        Result RawStartNode(AZStd::string_view name, Lifetime lifetime) override;
        Result EndNode(AZ::u64 attributeCount, AZ::u64 elementCount) override;

    private:
        void Initialize();
        Visitor::Result ValueBegin();
        Visitor::Result ValueEnd();
        Visitor::Result HandleValue(AZStd::function<Visitor::Result()> valueHandler);

        VisitorFlags m_supportToPolyfill = VisitorFlags::Null;
        Visitor* m_proxiedVisitor;
        AZStd::unique_ptr<Visitor> m_proxiedVisitorOwningStorage;

        struct NodeStackEntry
        {
            size_t m_attributeSize = 0;
            size_t m_elementSize = 0;
            AZ::Name m_key;
        };
        AZStd::stack<NodeStackEntry> m_nodeStack;

        struct EntryStackEntry
        {
            AZ::Name m_node;
            bool m_nextStringIsKey = false;
        };
        AZStd::stack<EntryStackEntry> m_entryStack;

        static AZ::Name s_objectNodeName;
        static AZ::Name s_objectNodeKeyAttributeName;
        static AZ::Name s_arrayNodeName;
        static AZ::Name s_entryNodeName;
        AZStd::regex m_quoteRegex;
        AZStd::regex m_trueRegex;
        AZStd::regex m_falseRegex;
        AZStd::regex m_nullRegex;
        AZStd::regex m_numberRegex;
    };
} // namespace AZ::Dom
