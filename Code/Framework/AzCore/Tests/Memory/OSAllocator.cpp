/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/OSAllocator.h>

namespace UnitTest
{
    TEST(OSAllocatorTest, TestAllocationDeallocation)
    {
        static const AZStd::pair<size_t, size_t> sizeAndAlignments[] =
        {
            { 16, 8 },
            { 15, 1 },
            { 17, 1 },
            { 1024, 16 },
            { 1023, 1 },
            { 1025, 1 },
            { 65536, 16 },
            { 65535, 1 },
            { 65537, 1 }
        };

        AZStd::fixed_vector<void*, AZ_ARRAY_SIZE(sizeAndAlignments)> records;
        AZ::OSAllocator osAllocator;

        // Allocate memory
        for (const auto& sizeAndAlignment : sizeAndAlignments)
        {
            void* p = osAllocator.allocate(sizeAndAlignment.first, sizeAndAlignment.second);
            EXPECT_NE(p, nullptr);
            records.push_back(p);
        }

        // Deallocate memory
        for (auto record : records)
        {
            m_mallocSchema.deallocate(record);
        }
    }

    TEST(OSAllocatorTest, TestReallocation)
    {
        static const AZStd::pair<size_t, size_t> sizeAndAlignments[] =
        {
            { 16, 8 },
            { 1024, 16 },
            { 32, 8 },
            { 0, 0 }
        };

        void* p = nullptr;
        
        // Keep reallocating the same pointer
        for (const auto& sizeAndAlignment : sizeAndAlignments)
        {
            p = m_mallocSchema.reallocate(p, sizeAndAlignment.first, sizeAndAlignment.second);

            if (sizeAndAlignment.first)
            {
                EXPECT_NE(p, nullptr);
            }
            else
            {
                EXPECT_EQ(p, nullptr);
            }
        }
    }
}
