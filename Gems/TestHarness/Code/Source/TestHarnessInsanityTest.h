/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/Component.h>
#include "TestHarness/TestCase.h"
#include "TestHarness/TestCaseRequestBus.h"
#include "TestHarness/TestCaseEventBus.h"

namespace TestHarness
{
    class TestHarnessInsanityTest
        : public AZ::Component
        , public TestHarness::TestCase
    {
    public:
        AZ_COMPONENT(TestHarnessInsanityTest, "{31F3691D-900E-4B35-BE72-0005C23BD43E}");

        static void Reflect(AZ::ReflectContext* context);

        TestHarnessInsanityTest() {}
        ~TestHarnessInsanityTest() override = default;

    protected:
        // AZ::Component 
        void Activate() override { TestCase::Activate(AZ::Component::GetEntityId()); }
        void Deactivate() override { TestCase::Deactivate(); }

        // TestHarness::TestCaseRequestBus
        void Setup() override;
        void TearDown() override;
        void Execute() override;
    };
}
