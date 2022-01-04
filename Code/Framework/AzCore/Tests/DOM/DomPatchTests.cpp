/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomPatch.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Tests
{
    class DomPatchTests : public DomTestFixture
    {
    public:
        void SetUp() override
        {
            DomTestFixture::SetUp();

            m_dataset = Value(Type::Object);
            m_dataset["arr"].SetArray();

            m_dataset["node"].SetNode("SomeNode");
            m_dataset["node"]["int"] = 5;
            m_dataset["node"]["null"] = Value();

            for (int i = 0; i < 5; ++i)
            {
                m_dataset["arr"].PushBack(i);
                m_dataset["node"].PushBack(i * 2);
            }

            m_dataset["obj"].SetObject();
            m_dataset["obj"]["foo"] = true;
            m_dataset["obj"]["bar"] = false;

            m_deltaDataset = m_dataset;
        }

        void TearDown() override
        {
            m_dataset = m_deltaDataset = Value();

            DomTestFixture::TearDown();
        }

        PatchInfo GenerateAndVerifyDelta()
        {
            PatchInfo info = GenerateHierarchicalDeltaPatch(m_dataset, m_deltaDataset);

            auto result = info.m_forwardPatches.Apply(m_dataset);
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_TRUE(result.GetValue().DeepCompareIsEqual(m_deltaDataset));

            result = info.m_inversePatches.Apply(result.GetValue());
            EXPECT_TRUE(result.IsSuccess());
            EXPECT_TRUE(result.GetValue().DeepCompareIsEqual(m_dataset));

            return info;
        }

        Value m_dataset;
        Value m_deltaDataset;
    };

    TEST_F(DomPatchTests, AddOperation_InsertInObject_Succeeds)
    {
        Path p("/obj/baz");
        PatchOperation op = Patch::AddOperation(p, 42);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetInt32(), 42);
    }

    TEST_F(DomPatchTests, AddOperation_ReplaceInObject_Succeeds)
    {
        Path p("/obj/foo");
        PatchOperation op = Patch::AddOperation(p, false);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetBool(), false);
    }

    TEST_F(DomPatchTests, AddOperation_InsertObjectKeyInArray_Fails)
    {
        Path p("/arr/key");
        PatchOperation op = Patch::AddOperation(p, 999);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, AddOperation_AppendInArray_Succeeds)
    {
        Path p("/arr/-");
        PatchOperation op = Patch::AddOperation(p, 42);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["arr"][5].GetInt32(), 42);
    }

    TEST_F(DomPatchTests, AddOperation_InsertKeyInNode_Succeeds)
    {
        Path p("/node/attr");
        PatchOperation op = Patch::AddOperation(p, 500);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetInt32(), 500);
    }

    TEST_F(DomPatchTests, AddOperation_ReplaceIndexInNode_Succeeds)
    {
        Path p("/node/0");
        PatchOperation op = Patch::AddOperation(p, 42);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetInt32(), 42);
    }

    TEST_F(DomPatchTests, AddOperation_AppendInNode_Succeeds)
    {
        Path p("/node/-");
        PatchOperation op = Patch::AddOperation(p, 42);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["node"][5].GetInt32(), 42);
    }

    TEST_F(DomPatchTests, AddOperation_InvalidPath_Fails)
    {
        Path p("/non/existent/path");
        PatchOperation op = Patch::AddOperation(p, 0);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveKeyFromObject_Succeeds)
    {
        Path p("/obj/foo");
        PatchOperation op = Patch::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_FALSE(result.GetValue()["obj"].HasMember("foo"));
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveIndexFromArray_Succeeds)
    {
        Path p("/arr/0");
        PatchOperation op = Patch::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["arr"].Size(), 4);
        EXPECT_EQ(result.GetValue()["arr"][0].GetInt32(), 1);
    }

    TEST_F(DomPatchTests, RemoveOperation_PopArray_Succeeds)
    {
        Path p("/arr/-");
        PatchOperation op = Patch::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        EXPECT_EQ(result.GetValue()["arr"].Size(), 4);
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveKeyFromNode_Succeeds)
    {
        Path p("/node/int");
        PatchOperation op = Patch::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_FALSE(result.GetValue()["node"].HasMember("int"));
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveIndexFromNode_Succeeds)
    {
        Path p("/node/1");
        PatchOperation op = Patch::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["node"].Size(), 4);
        EXPECT_EQ(result.GetValue()["node"][1].GetInt32(), 4);
    }

    TEST_F(DomPatchTests, RemoveOperation_PopIndexFromNode_Succeeds)
    {
        Path p("/node/-");
        PatchOperation op = Patch::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()["node"].Size(), 4);
    }

    TEST_F(DomPatchTests, RemoveOperation_RemoveKeyFromArray_Fails)
    {
        Path p("/arr/foo");
        PatchOperation op = Patch::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, RemoveOperation_InvalidPath_Fails)
    {
        Path p("/non/existent/path");
        PatchOperation op = Patch::RemoveOperation(p);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_InsertInObject_Fails)
    {
        Path p("/obj/baz");
        PatchOperation op = Patch::ReplaceOperation(p, 42);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_ReplaceInObject_Succeeds)
    {
        Path p("/obj/foo");
        PatchOperation op = Patch::ReplaceOperation(p, false);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetBool(), false);
    }

    TEST_F(DomPatchTests, ReplaceOperation_InsertObjectKeyInArray_Fails)
    {
        Path p("/arr/key");
        PatchOperation op = Patch::ReplaceOperation(p, 999);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_AppendInArray_Fails)
    {
        Path p("/arr/-");
        PatchOperation op = Patch::ReplaceOperation(p, 42);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_InsertKeyInNode_Fails)
    {
        Path p("/node/attr");
        PatchOperation op = Patch::ReplaceOperation(p, 500);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_ReplaceIndexInNode_Succeeds)
    {
        Path p("/node/0");
        PatchOperation op = Patch::ReplaceOperation(p, 42);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_EQ(result.GetValue()[p].GetInt32(), 42);
    }

    TEST_F(DomPatchTests, ReplaceOperation_AppendInNode_Fails)
    {
        Path p("/node/-");
        PatchOperation op = Patch::ReplaceOperation(p, 42);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, ReplaceOperation_InvalidPath_Fails)
    {
        Path p("/non/existent/path");
        PatchOperation op = Patch::ReplaceOperation(p, 0);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, CopyOperation_ArrayToObject_Succeeds)
    {
        Path dest("/obj/arr");
        Path src("/arr");
        PatchOperation op = Patch::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(m_dataset[src].DeepCompareIsEqual(result.GetValue()[dest]));
        EXPECT_TRUE(result.GetValue()[src].DeepCompareIsEqual(result.GetValue()[dest]));
    }

    TEST_F(DomPatchTests, CopyOperation_ObjectToArrayInRange_Succeeds)
    {
        Path dest("/arr/0");
        Path src("/obj");
        PatchOperation op = Patch::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(m_dataset[src].DeepCompareIsEqual(result.GetValue()[dest]));
        EXPECT_TRUE(result.GetValue()[src].DeepCompareIsEqual(result.GetValue()[dest]));
    }

    TEST_F(DomPatchTests, CopyOperation_ObjectToArrayOutOfRange_Fails)
    {
        Path dest("/arr/5");
        Path src("/obj");
        PatchOperation op = Patch::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, CopyOperation_ObjectToNodeChildInRange_Succeeds)
    {
        Path dest("/node/0");
        Path src("/obj");
        PatchOperation op = Patch::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(m_dataset[src].DeepCompareIsEqual(result.GetValue()[dest]));
        EXPECT_TRUE(result.GetValue()[src].DeepCompareIsEqual(result.GetValue()[dest]));
    }

    TEST_F(DomPatchTests, CopyOperation_ObjectToNodeChildOutOfRange_Fails)
    {
        Path dest("/node/5");
        Path src("/obj");
        PatchOperation op = Patch::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, CopyOperation_InvalidSourcePath_Fails)
    {
        Path dest("/node/0");
        Path src("/invalid/path");
        PatchOperation op = Patch::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, CopyOperation_InvalidDestinationPath_Fails)
    {
        Path dest("/invalid/path");
        Path src("/arr/0");
        PatchOperation op = Patch::CopyOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, MoveOperation_ArrayToObject_Succeeds)
    {
        Path dest("/obj/arr");
        Path src("/arr");
        PatchOperation op = Patch::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(m_dataset[src].DeepCompareIsEqual(result.GetValue()[dest]));
        EXPECT_FALSE(result.GetValue().HasMember("arr"));
    }

    TEST_F(DomPatchTests, MoveOperation_ObjectToArrayInRange_Succeeds)
    {
        Path dest("/arr/0");
        Path src("/obj");
        PatchOperation op = Patch::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(m_dataset[src].DeepCompareIsEqual(result.GetValue()[dest]));
        EXPECT_FALSE(result.GetValue().HasMember("obj"));
    }

    TEST_F(DomPatchTests, MoveOperation_ObjectToArrayOutOfRange_Fails)
    {
        Path dest("/arr/5");
        Path src("/obj");
        PatchOperation op = Patch::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, MoveOperation_ObjectToNodeChildInRange_Succeeds)
    {
        Path dest("/node/0");
        Path src("/obj");
        PatchOperation op = Patch::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
        EXPECT_TRUE(m_dataset[src].DeepCompareIsEqual(result.GetValue()[dest]));
        EXPECT_FALSE(result.GetValue().HasMember("obj"));
    }

    TEST_F(DomPatchTests, MoveOperation_ObjectToNodeChildOutOfRange_Fails)
    {
        Path dest("/node/5");
        Path src("/obj");
        PatchOperation op = Patch::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, MoveOperation_InvalidSourcePath_Fails)
    {
        Path dest("/node/0");
        Path src("/invalid/path");
        PatchOperation op = Patch::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, MoveOperation_InvalidDestinationPath_Fails)
    {
        Path dest("/invalid/path");
        Path src("/arr/0");
        PatchOperation op = Patch::MoveOperation(dest, src);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestCorrectValue_Succeeds)
    {
        Path path("/arr/1");
        Value value = 1;
        PatchOperation op = Patch::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestIncorrectValue_Fails)
    {
        Path path("/arr/1");
        Value value = 55;
        PatchOperation op = Patch::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestCorrectComplexValue_Succeeds)
    {
        Path path;
        Value value = m_dataset;
        PatchOperation op = Patch::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_TRUE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestIncorrectComplexValue_Fails)
    {
        Path path;
        Value value = m_dataset;
        value["arr"][4] = 9;
        PatchOperation op = Patch::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestInvalidPath_Fails)
    {
        Path path("/invalid/path");
        Value value;
        PatchOperation op = Patch::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, TestOperation_TestInsertArrayPath_Fails)
    {
        Path path("/arr/-");
        Value value = 4;
        PatchOperation op = Patch::TestOperation(path, value);
        auto result = op.Apply(m_dataset);
        ASSERT_FALSE(result.IsSuccess());
    }

    TEST_F(DomPatchTests, Test_Patch_ReplaceArrayValue)
    {
        m_deltaDataset["arr"][0] = 5;
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_AppendArrayValue)
    {
        m_deltaDataset["arr"].PushBack(7);
        auto result = GenerateAndVerifyDelta();

        // Ensure the generated patch uses the array append operation
        ASSERT_EQ(result.m_forwardPatches.Size(), 1);
        EXPECT_TRUE(result.m_forwardPatches[0].GetDestinationPath()[1].IsEndOfArray());
    }

    TEST_F(DomPatchTests, Test_Patch_AppendArrayValues)
    {
        m_deltaDataset["arr"].PushBack(7);
        m_deltaDataset["arr"].PushBack(8);
        m_deltaDataset["arr"].PushBack(9);
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_InsertArrayValue)
    {
        auto& arr = m_deltaDataset["arr"].GetMutableArray();
        arr.insert(arr.begin(), Value(42));
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_InsertObjectKey)
    {
        m_deltaDataset["obj"]["newKey"].CopyFromString("test");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_DeleteObjectKey)
    {
        m_deltaDataset["obj"].RemoveMember("foo");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_AppendNodeValues)
    {
        m_deltaDataset["node"].PushBack(7);
        m_deltaDataset["node"].PushBack(8);
        m_deltaDataset["node"].PushBack(9);
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_InsertNodeValue)
    {
        auto& node = m_deltaDataset["node"].GetMutableNode();
        node.GetChildren().insert(node.GetChildren().begin(), Value(42));
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_InsertNodeKey)
    {
        m_deltaDataset["node"]["newKey"].CopyFromString("test");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_DeleteNodeKey)
    {
        m_deltaDataset["node"].RemoveMember("int");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_RenameNode)
    {
        m_deltaDataset["node"].SetNodeName("RenamedNode");
        GenerateAndVerifyDelta();
    }

    TEST_F(DomPatchTests, Test_Patch_ReplaceRoot)
    {
        m_deltaDataset = Value(Type::Array);
        m_deltaDataset.PushBack(2);
        m_deltaDataset.PushBack(4);
        m_deltaDataset.PushBack(6);
        GenerateAndVerifyDelta();
    }
} // namespace AZ::Dom::Tests
