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

#include "TestHarness/TestCaseEventBus.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace TestHarness
{
    class TestCaseEventLuaHandler
        : public AZ::BehaviorEBusHandler
        , public TestCaseEventBus::Handler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(TestCaseEventLuaHandler, "{ACBE714E-D8A5-4F5D-96B1-B3B121E6E6E8}", AZ::SystemAllocator,
            Completed,
            ExpectTrue,
            SetupCompleted);

        void Completed() override
        {
            Call(FN_Completed);
        }

        void ExpectTrue(bool actual) override
        {
            Call(FN_ExpectTrue, actual);
        }

        void SetupCompleted() override
        {
            Call(FN_SetupCompleted);
        }
    };

    void ReflectTestCaseEventBus(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<TestCaseEventBus>("TestCaseEventBus")
                ->Handler<TestCaseEventLuaHandler>()
                ->Event("Completed", &TestCaseEventBus::Events::Completed)
                ->Event("ExpectTrue", &TestCaseEventBus::Events::ExpectTrue)
                ->Event("SetupCompleted", &TestCaseEventBus::Events::SetupCompleted)
                ;
        }
    }
} // namespace TestHarness
