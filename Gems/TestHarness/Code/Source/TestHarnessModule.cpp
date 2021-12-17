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

#include <TestHarnessModuleInterface.h>
#include <Components/TestHarnessSystemComponent.h>

//TODO were these includes ever important?
//#include "Components/TestCaseComponent.h"
//#include "Components/TestHarnessSystemComponent.h"
//#include "TestHarnessSanityTest.h"
//#include "TestHarnessInsanityTest.h"

 namespace TestHarness
{
    class TestHarnessModule
        : public TestHarnessModuleInterface
    {
    public:
        AZ_RTTI(TestHarnessModule, "{6F0047DE-95DC-48EC-B078-23F09CD737EC}", AZ::Module); //TODO module interface
        AZ_CLASS_ALLOCATOR(TestHarnessModule, AZ::SystemAllocator, 0);
    };
}// namespace TestHarness

AZ_DECLARE_MODULE_CLASS(Gem_TestHarness, TestHarness::TestHarnessModule)
