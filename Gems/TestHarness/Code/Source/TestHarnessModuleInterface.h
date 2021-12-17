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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Components/TestHarnessSystemComponent.h>

namespace TestHarness
{
	class TestHarnessModuleInterface
		: public AZ::Module
	{
	public:
		AZ_RTTI(TestHarnessModule, "{6F0047DE-95DC-48EC-B078-23F09CD737EC}", AZ::Module);
        AZ_CLASS_ALLOCATOR(TestHarnessModuleInterface, AZ::SystemAllocator, 0);

		TestHarnessModuleInterface()
        {
            m_descriptors.insert(
                m_descriptors.end(), {
                    TestHarnessSystemComponent::CreateDescriptor(),
                    });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<TestHarnessSystemComponent>(),
            };
        }
	};
}// namespace TestHarness
