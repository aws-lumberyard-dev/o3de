/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/DocumentPropertyEditor/DocumentPropertyEditorFixture.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzCore/DOM/DomUtils.h>

namespace AZ::DocumentPropertyEditor::Tests
{
    using AdapterBuilderTests = DocumentPropertyEditorTestFixture;

    TEST_F(AdapterBuilderTests, VisitSimpleStructure)
    {
        AdapterBuilder builder(*m_adapter);
        builder.BeginRow();
        builder.BeginLabel("label", true);
        builder.EndLabel();
        builder.BeginPropertyEditor(AZ_NAME_LITERAL("editor"));
        builder.Attribute(AZ_NAME_LITERAL("attr"), Dom::Value(2));
        builder.EndPropertyEditor();
        builder.EndRow();

        Dom::Value result = builder.TakeValue();

        /**
        Expect the following structure:
        <Adapter>
            <Row>
                <Label>label</Label>
                <editor attr=2 />
            </Row>
        </Adapter>
        */
        Dom::Value expectedValue = Dom::Value::CreateNode("Adapter");
        Dom::Value row = Dom::Value::CreateNode("Row");
        Dom::Value label = Dom::Value::CreateNode("Label");
        label.SetNodeValue(Dom::Value("label", true));
        row.ArrayPushBack(label);
        Dom::Value editor = Dom::Value::CreateNode("editor");
        editor["attr"] = 2;
        row.ArrayPushBack(editor);
        expectedValue.ArrayPushBack(row);

        EXPECT_TRUE(Dom::Utils::DeepCompareIsEqual(expectedValue, result));
    }
}
