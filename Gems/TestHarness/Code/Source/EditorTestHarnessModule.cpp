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
#include <Editor/EditorTestHarnessSystemComponent.h>
//#include "TestHarnessSanityTest.h"
//#include "TestHarnessInsanityTest.h"
//#include "TestHarnessDestroyPyramidTest.h"

//TODO additional includes necessary?
//#include "Components/TestHarnessSystemComponent.h"
//#include "Editor/EditorTestCaseComponent.h"

namespace TestHarness
{
    class EditorTestHarnessModule
        : public TestHarnessModuleInterface
    {
    public:
        AZ_RTTI(EditorTestHarnessModule, "{FF50ED33-00F8-492B-87E7-1CE6D1B2EC55}", TestHarnessModuleInterface);
        AZ_CLASS_ALLOCATOR(EditorTestHarnessModule, AZ::SystemAllocator, 0);

        EditorTestHarnessModule()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    //EditorTestHarnessSystemComponent::CreateDescriptor(), EditorTestCaseComponent::CreateDescriptor(),

                    /*
                    Below are all C++ tests that will be registered as components.
                    This step is currently necessary for C++ test registration.
                    TODO: fix this manual registration pattern?
                    TODO: issue with which module these are registered into?
                    */
                    //TestHarnessSanityTest::CreateDescriptor(), TestHarnessInsanityTest::CreateDescriptor(),
                    //TestHarnessDestroyPyramidTest::CreateDescriptor()
                    EditorTestHarnessSystemComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<EditorTestHarnessSystemComponent>(),
            };
        }
    };
} // namespace TestHarness

AZ_DECLARE_MODULE_CLASS(Gem_TestHarness, TestHarness::EditorTestHarnessModule)
