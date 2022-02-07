/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/TestTypes.h>
#include <AzCore/Memory/AllocatorInstance.h>
#include <AzCore/Memory/AllocatorWrappers.h>

namespace UnitTest
{
    class TestMemoryAllocator : public AZ::OSAllocator
    {
    public:
        AZ_RTTI(TestMemoryAllocator, "{EEDC55B9-8E3F-465E-944E-84C76D5F2AB3}", AZ::OSAllocator);

        // Required to be able to move data from static instances to environment. We could make this optional and those allocators would
        // fail to be used before the environment is ready.
        void Merge(IAllocator* aOther) override
        {
            TestMemoryAllocator* other = azrtti_cast<TestMemoryAllocator*>(aOther);

            // This is where data from aOther will be moved to "this"
            // For the test we simulate allocations being passed from one allocator to the other
            m_hasAllocations |= other->m_hasAllocations;
            other->m_hasAllocations = false;
        }

        bool m_hasAllocations = false;
    };
    
    TEST(AllocatorInstance, Create)
    {
        EXPECT_TRUE(AZ::Environment::IsReady());
        AZ::Environment::Destroy();
        EXPECT_FALSE(AZ::Environment::IsReady());

        // Create an allocator before the environemnt is available
        TestMemoryAllocator& allocatorStatic1 = AZ::AllocatorInstance<TestMemoryAllocator>::Get();
        TestMemoryAllocator& allocatorStatic2 = AZ::AllocatorInstance<TestMemoryAllocator>::Get();
        EXPECT_EQ(&allocatorStatic1, &allocatorStatic2);
        EXPECT_FALSE(allocatorStatic1.m_hasAllocations);
        allocatorStatic1.m_hasAllocations = true; // simulate an allocation, just to check that the instance is preserved

        AZ::Environment::Create();
        EXPECT_TRUE(AZ::Environment::IsReady());

        TestMemoryAllocator& allocatorEnvironment1 = AZ::AllocatorInstance<TestMemoryAllocator>::Get();
        TestMemoryAllocator& allocatorEnvironment2 = AZ::AllocatorInstance<TestMemoryAllocator>::Get();
        EXPECT_EQ(&allocatorEnvironment1, &allocatorEnvironment2);
        EXPECT_TRUE(allocatorStatic1.m_hasAllocations);

        EXPECT_TRUE(allocatorEnvironment1.m_hasAllocations); // Should have been transferred from allocatorStatic
        EXPECT_TRUE(allocatorStatic1.m_hasAllocations);
        EXPECT_EQ(&allocatorStatic1, &allocatorEnvironment1); // same allocator between static and environment

        AZ::Environment::Destroy(); // Destroy the environment, the allocators should still be available

        EXPECT_TRUE(allocatorStatic1.m_hasAllocations); // Still valid after environment is destroyed (will be in static memory)

        AZ::Environment::Create(); // restore it for other tests
    }
}
