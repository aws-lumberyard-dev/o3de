/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/Xml/XmlBackend.h>
#include <AzCore/DOM/DomPolyfillBackend.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Tests
{
    class DomXmlTests : public DomTestFixture
    {
    public:
        void CompareXmlToValue(AZStd::string_view xml, Dom::Value value)
        {
            DomPolyfillBackend<XmlBackend> backend;

            Value deserializedValue;
            Utils::ReadFromString(backend, xml, Lifetime::Persistent, *deserializedValue.GetWriteHandler());

            EXPECT_TRUE(Utils::DeepCompareIsEqual(value, deserializedValue));

            AZStd::string serializedXml;
            backend.WriteToBuffer(
                serializedXml,
                [&](Dom::Visitor& visitor)
                {
                    return value.Accept(visitor, false);
                });
            Utils::ReadFromString(backend, serializedXml, Lifetime::Persistent, *deserializedValue.GetWriteHandler());

            EXPECT_TRUE(Utils::DeepCompareIsEqual(value, deserializedValue));
        }
    };

    TEST_F(DomXmlTests, SingleElement)
    {
        Dom::Value value;
        value.SetNode("Element");

        CompareXmlToValue("<Element />", value);
    }

    TEST_F(DomXmlTests, AttributesRestoredAsStrings)
    {
        Dom::Value value = Value::CreateNode("ElementWithAttrs");
        value["int64"] = -1;
        value["uint64"] = 42ull;
        value["bool"] = true;
        value["null"] = Dom::Value();
        value["str"] = Dom::Value("string", false);

        XmlBackend backend;
        AZStd::string serializedXml;
        backend.WriteToBuffer(
            serializedXml,
            [&](Visitor& visitor)
            {
                return value.Accept(visitor, false);
            });
        Value deserializedValue;
        backend.ReadFromBuffer(serializedXml.data(), serializedXml.length(), Lifetime::Persistent, *deserializedValue.GetWriteHandler());

        EXPECT_EQ(deserializedValue["int64"].GetString(), "-1");
        EXPECT_EQ(deserializedValue["uint64"].GetString(), "42");
        EXPECT_EQ(deserializedValue["bool"].GetString(), "true");
        EXPECT_EQ(deserializedValue["null"].GetString(), "null");
        EXPECT_EQ(deserializedValue["str"].GetString(), "string");
    }

    TEST_F(DomXmlTests, ElementWithPolyfilledAttributes)
    {
        Dom::Value value = Value::CreateNode("ElementWithAttrs");
        value["int64"] = -1;
        value["uint64"] = 42ull;
        value["bool"] = true;
        value["null"] = Dom::Value();
        value["str"] = Dom::Value("string", false);

        CompareXmlToValue(R"(<ElementWithAttrs int64="-1" uint64="42" bool="true" null="null" str="string" />))", value);
    }

    TEST_F(DomXmlTests, NestedElements)
    {
        Value root = Value::CreateNode("Root");
        for (size_t i = 0; i < 5; ++i)
        {
            Value child = Value::CreateNode("Child");
            for (size_t c = 0; c < 3; ++c)
            {
                Value grandchild = Value::CreateNode("Grandchild");
                grandchild["idx"] = c;
            }
        }

        CompareXmlToValue(
            R"(
            <Root>
                <Child>
                    <Grandchild idx="0" />
                    <Grandchild idx="1" />
                    <Grandchild idx="2" />
                </Child>
                <Child>
                    <Grandchild idx="0" />
                    <Grandchild idx="1" />
                    <Grandchild idx="2" />
                </Child>
                <Child>
                    <Grandchild idx="0" />
                    <Grandchild idx="1" />
                    <Grandchild idx="2" />
                </Child>
                <Child>
                    <Grandchild idx="0" />
                    <Grandchild idx="1" />
                    <Grandchild idx="2" />
                </Child>
                <Child>
                    <Grandchild idx="0" />
                    <Grandchild idx="1" />
                    <Grandchild idx="2" />
                </Child>
            </Root>
            )",
            root);
    }

    TEST_F(DomXmlTests, Arrays)
    {
        Value root = Value::CreateNode("Root");
        Value arr(Type::Array);
        for (size_t i = 0; i < 5; ++i)
        {
            arr.ArrayPushBack(Value(i));
        }
        root["arr"] = arr;

        CompareXmlToValue(
            R"(
            <Root>
                <o3de:Array o3de:Key="arr">
                    <o3de:Entry>0</o3de:Entry>
                    <o3de:Entry>1</o3de:Entry>
                    <o3de:Entry>2</o3de:Entry>
                    <o3de:Entry>3</o3de:Entry>
                    <o3de:Entry>4</o3de:Entry>
                </o3de:Array>
            </Root>
            )",
            root);
    }
} // namespace AZ::Dom::Tests
