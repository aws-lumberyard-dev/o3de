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

#include "TestHarnessSanityTest.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace TestHarness
{
    void TestHarnessSanityTest::Reflect(AZ::ReflectContext * context)
    {
        if (TH_CONTEXT_SERIALIZE) // Serialization
        {
            TH_SERIALIZE(TestHarnessSanityTest);

            if (TH_CONTEXT_EDIT) // Editor
            {
                TH_REFLECT(TestHarnessSanityTest, "TestHarness Sanity Test", "TestHarness Sanity Test");
            }
        }
    }

    void TestHarnessSanityTest::Setup()
    {
        TH_SETUP_COMPLETED;
    }

    void TestHarnessSanityTest::Execute()
    {
        TH_EXPECT_TRUE(true);
        TH_COMPLETE_TEST;
    }

    void TestHarnessSanityTest::TearDown()
    {
    }
}
