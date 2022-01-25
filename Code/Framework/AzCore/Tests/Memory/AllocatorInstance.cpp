/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/AllocatorInstance.h>

namespace UnitTest
{
    class TestMemoryAllocator // Example Allocator: just a class to use for testing
    {
    public:
        AZ_TYPE_INFO(TestMemoryAllocator, "{EEDC55B9-8E3F-465E-944E-84C76D5F2AB3}");

        class Descriptor
        {
        };

        // Required to be able to move data from static instances to environment. We could make this optional and those allocators would
        // fail to be used before the environment is ready.
        void Merge(TestMemoryAllocator* aOther) 
        {
            // This is where data from aOther will be moved to "this"
            // For the test we simulate allocations being passed from one allocator to the other
            m_hasAllocations |= aOther->m_hasAllocations;
            aOther->m_hasAllocations = false;
        }


        bool m_hasAllocations = false;
    };
    
    TEST(AllocatorInstance, Create)
    {
        EXPECT_TRUE(AZ::Environment::IsReady());
        AZ::Environment::Destroy();
        EXPECT_FALSE(AZ::Environment::IsReady());

        TestMemoryAllocator& allocatorStatic1 = AZ::AllocatorInstance<TestMemoryAllocator>::Get();
        TestMemoryAllocator& allocatorStatic2 = AZ::AllocatorInstance<TestMemoryAllocator>::Get();
        EXPECT_EQ(&allocatorStatic1, &allocatorStatic2);
        allocatorStatic1.m_hasAllocations = true;

        AZ::Environment::Create(nullptr);
        EXPECT_TRUE(AZ::Environment::IsReady());

        TestMemoryAllocator& allocatorEnvironment1 = AZ::AllocatorInstance<TestMemoryAllocator>::Get();
        TestMemoryAllocator& allocatorEnvironment2 = AZ::AllocatorInstance<TestMemoryAllocator>::Get();
        EXPECT_EQ(&allocatorEnvironment1, &allocatorEnvironment2);

        EXPECT_TRUE(allocatorEnvironment1.m_hasAllocations); // Should have been transferred from allocatorStatic
        EXPECT_FALSE(allocatorStatic1.m_hasAllocations);

    }
}
