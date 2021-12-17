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
* Test that always passes regardless. Used for debug purposes. Attach to an entity
* alongside the 'TestCaseComponent' component in order for the test ot be registered.
*
*/
#pragma once

#include "TestHarness/TestCase.h"
#include "TestHarness/TestCaseEventBus.h"
#include "TestHarness/TestCaseRequestBus.h"
#include <AzCore/Component/Component.h>
#include <ISystem.h>

namespace TestHarness
{
    class TestHarnessSanityTest
        : public AZ::Component
        , public TestHarness::TestCase
    {
    public:
        AZ_COMPONENT(TestHarnessSanityTest, "{3A1B41E5-96C9-4AF8-BD62-E85B11B2D719}");

        static void Reflect(AZ::ReflectContext* context);

        TestHarnessSanityTest() {}
        ~TestHarnessSanityTest() override = default;

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
