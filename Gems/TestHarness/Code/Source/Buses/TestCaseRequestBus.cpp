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

#include "TestHarness/TestCaseRequestBus.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace TestHarness
{
    class TestCaseRequestLuaHandler
        : public AZ::BehaviorEBusHandler
        , public TestCaseRequestBus::Handler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(TestCaseRequestLuaHandler, "{07A40135-15C5-4693-BDC0-45151BF3D525}", AZ::SystemAllocator,
            Setup, TearDown, Execute);

        void Setup() override
        {
            Call(FN_Setup);
        }
        void TearDown() override
        {
            Call(FN_TearDown);
        }
        void Execute() override
        {
            Call(FN_Execute);
        }
    };

    void ReflectTestCaseRequestBus(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<TestCaseRequestBus>("TestCaseRequestBus")
                ->Handler<TestCaseRequestLuaHandler>()
                ->Event("Setup", &TestCaseRequestBus::Events::Setup)
                ->Event("TearDown", &TestCaseRequestBus::Events::TearDown)
                ->Event("Execute", &TestCaseRequestBus::Events::Execute)
                ;
        }
    }
} // namespace TestHarness
