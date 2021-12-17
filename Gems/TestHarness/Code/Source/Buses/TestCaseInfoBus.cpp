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

#include "TestHarness/TestCaseInfoBus.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace TestHarness
{
    class TestCaseInfoLuaHandler
        : public AZ::BehaviorEBusHandler
        , public TestCaseInfoBus::Handler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(TestCaseInfoLuaHandler, "{82655078-AC65-492E-B8E1-7254CB6199DE}", AZ::SystemAllocator,
            GetEntityId, GetTestCaseName, IsEnabled, GetUserDefinedTags);

        AZ::EntityId GetEntityId() override
        {
            AZ::EntityId result;
            CallResult(result, FN_GetEntityId);
            return result;
        }
        AZStd::string GetTestCaseName() override
        {
            AZStd::string result;
            CallResult(result, FN_GetTestCaseName);
            return result;
        }
        bool IsEnabled() override
        {
            bool result = false;
            CallResult(result, FN_IsEnabled);
            return result;
        }
        AZStd::set<AZStd::string> GetUserDefinedTags() override
        {
            AZStd::set<AZStd::string> result;
            CallResult(result, FN_GetUserDefinedTags);
            return result;
        }
    };

    void ReflectTestCaseInfoBus(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<TestCaseInfoBus>("TestCaseInfoBus")
                ->Handler<TestCaseInfoLuaHandler>()
                ->Event("GetEntityId", &TestCaseInfoBus::Events::GetEntityId)
                ->Event("GetTestCaseName", &TestCaseInfoBus::Events::GetTestCaseName)
                ->Event("IsEnabled", &TestCaseInfoBus::Events::IsEnabled)
                ->Event("GetUserDefinedTags", &TestCaseInfoBus::Events::GetUserDefinedTags)
                ;
        }
    }
} // namespace TestHarness
