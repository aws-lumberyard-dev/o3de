/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/UnitTest.h>

#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
#include <csignal>
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/OSAllocator.h>

namespace UnitTest
{
    void TraceBusHook::SetupEnvironment()
    {
#if AZ_TRAIT_UNITTEST_USE_TEST_RUNNER_ENVIRONMENT
        AZ::EnvironmentInstance inst = AZ::Test::GetPlatform().GetTestRunnerEnvironment();
        AZ::Environment::Attach(inst);
#else
        AZ::Environment::Create();
#endif
        BusConnect();

        m_environmentSetup = true;
    }

    void TraceBusHook::TeardownEnvironment()
    {
        if (m_environmentSetup)
        {
            BusDisconnect();

            // Leak detection. We need to collect the allocators and then shutdown the environment to remove all
            // variables that are there before we can detect leaks.
            AZStd::vector<AZ::IAllocator*, AZStd::stateless_allocator> allocators;
            {
                AZ::AllocatorManager& allocatorManager = AZ::AllocatorManager::Instance();
                const int numAllocators = static_cast<int>(allocatorManager.GetNumAllocators());
                // Iterate in reverse order since some allocators could depend on others to do garbage collection.
                // If allocatorB depends on allocatorA, allocatorA will be registered before into the allocator manager.
                for (int i = numAllocators - 1; i >= 0; --i)
                {
                    AZ::IAllocator* allocator = allocatorManager.GetAllocator(i);
                    allocator->GarbageCollect();
                    allocators.push_back(allocator);
                }
            }

#if AZ_TRAIT_UNITTEST_USE_TEST_RUNNER_ENVIRONMENT
            AZ::Environment::Detach();
#else
            AZ::Environment::Destroy();
#endif

            bool allocationsLeft = false;
            bool alocatorsWithoutTrackingLeft = false;

            // First find if there are any allocators without tracking left, we just warn on those. Since we dont know
            // how many allocations they did, we should not have them leaking at this point.
            for (AZ::IAllocator* allocator : allocators)
            {
                AZ::IAllocatorTrackingRecorder* allocatorWithTracking = azrtti_cast<AZ::IAllocatorTrackingRecorder*>(allocator);
                if (!allocatorWithTracking)
                {
                    if (!alocatorsWithoutTrackingLeft)
                    {
                        ColoredPrintf(COLOR_YELLOW, "[     WARN ] There are still allocators without tracking registered\n");
                        alocatorsWithoutTrackingLeft = true;
                    }
                    ColoredPrintf(COLOR_YELLOW, "\t\t%s\n", allocator->GetName());
                }
            }
            // Second, fail with errors if any of the ones with tracking have allocations left
            for (AZ::IAllocator* allocator : allocators)
            {
                AZ::IAllocatorTrackingRecorder* allocatorWithTracking = azrtti_cast<AZ::IAllocatorTrackingRecorder*>(allocator);
                if (allocatorWithTracking && (allocatorWithTracking->GetRequested() > 0 || allocatorWithTracking->GetAllocated() > 0))
                {
                    if (!allocationsLeft)
                    {
                        ColoredPrintf(COLOR_RED, "[     FAIL ] There are still allocations\n");
                        allocationsLeft = true;
                    }
                    ColoredPrintf(
                        COLOR_RED, "\t\t%s, Request size left: %zu bytes, Allocated size left: %zu bytes\n", allocator->GetName(),
                        allocatorWithTracking->GetRequested(), allocatorWithTracking->GetAllocated());
                    allocatorWithTracking->PrintAllocations();
                }
            }
            if (allocationsLeft)
            {
                m_environmentSetup = false;
#if AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
                std::raise(SIGTERM);
#endif // AZ_TRAIT_COMPILER_SUPPORT_CSIGNAL
            }

            m_environmentSetup = false;
        }
    }

} // namespace UnitTest
