/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>
#include "BuilderManagerTests.h"
#include <AzCore/std/smart_ptr/make_shared.h>
#include <native/connection/connectionManager.h>

namespace UnitTests
{
    int TestBuilder::s_startupDelayMs = 0;

    class BuilderManagerTest : public UnitTest::ScopedAllocatorSetupFixture
    {

    };

    TEST_F(BuilderManagerTest, GetBuilder_ReservesFirstBuilderForCreateJobs)
    {
        ConnectionManager cm{nullptr};
        TestBuilderManager bm(&cm);

        // We start off with 1 builder pre-created
        ASSERT_EQ(bm.GetBuilderCreationCount(), 1);

        // Save off the uuid of the CreateJobs builder for later
        auto createJobsBuilderUuid = bm.GetBuilder(BuilderPurpose::CreateJobs)->GetUuid();

        constexpr int NumberOfBuilders = 15;
        AZStd::vector<BuilderRef> builders;

        for (int i = 0; i < NumberOfBuilders; ++i)
        {
            builders.push_back(bm.GetBuilder(AssetProcessor::BuilderPurpose::ProcessJob));
        }

        // There should now be NumberOfBuilders + 1 builders, because the first one is reserved for CreateJobs
        ASSERT_EQ(bm.GetBuilderCreationCount(), NumberOfBuilders + 1);

        // Now if we request a CreateJob builder, we should get the same builder again
        ASSERT_EQ(bm.GetBuilder(AssetProcessor::BuilderPurpose::CreateJobs)->GetUuid(), createJobsBuilderUuid);

        // And the number of builders should remain the same
        ASSERT_EQ(bm.GetBuilderCreationCount(), NumberOfBuilders + 1);

        // Release the builders and check that we still get the same builder for CreateJobs
        builders = {};

        ASSERT_EQ(bm.GetBuilder(AssetProcessor::BuilderPurpose::CreateJobs)->GetUuid(), createJobsBuilderUuid);
        ASSERT_EQ(bm.GetBuilderCreationCount(), NumberOfBuilders + 1);
    }

    AZ::Outcome<void, AZStd::string> TestBuilder::Start(AssetProcessor::BuilderPurpose /*purpose*/)
    {
        if (TestBuilder::s_startupDelayMs > 0)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(TestBuilder::s_startupDelayMs));
        }

        return AZ::Success();
    }

    TestBuilderManager::TestBuilderManager(ConnectionManager* connectionManager): BuilderManager(connectionManager)
    {
        TestBuilderManager::AddNewBuilder(BuilderPurpose::CreateJobs);
    }

    int TestBuilderManager::GetBuilderCreationCount() const
    {
        return m_connectionCounter;
    }

    AZStd::shared_ptr<AssetProcessor::Builder> TestBuilderManager::AddNewBuilder(BuilderPurpose purpose)
    {
        auto uuid = AZ::Uuid::CreateRandom();
        auto builder = AZStd::make_shared<TestBuilder>(m_quitListener, uuid, ++m_connectionCounter);

        m_builderList.AddBuilder(builder, purpose);

        return builder;
    }

    TEST_F(BuilderManagerTest, GetBuilder_DoesntBlockOnCreation)
    {
        // Tests that requests to GetBuilder don't block during process startup

        ConnectionManager connectionManager{ nullptr };
        TestBuilderManager builderManager(&connectionManager);

        constexpr int NumberOfThreads = 20;

        // Set an artificial process start up delay
        TestBuilder::s_startupDelayMs = 20;

        // Wait just slightly longer than the start up time
        constexpr int WaitIntervalMs = 30;

        AZStd::vector<AZStd::thread> threads;
        AZStd::atomic_int numRemainingThreads = NumberOfThreads;
        AZStd::binary_semaphore doneSignal;

        // Hold on to the builder refs we get so they don't get re-used
        AZStd::array<BuilderRef, NumberOfThreads> builders;

        for (int i = 0; i < NumberOfThreads; ++i)
        {
            threads.push_back(AZStd::thread(
                [&numRemainingThreads, &doneSignal, &builders, i]()
                {
                    auto* builderManagerInterface = AZ::Interface<IBuilderManagerRequests>::Get();

                    if (!builderManagerInterface)
                    {
                        AZ_Assert(false, "Coding error: BuilderManager interface is not available");
                        return;
                    }

                    BuilderRef builderRef = builderManagerInterface->GetBuilder(BuilderPurpose::ProcessJob);

                    builders[i] = AZStd::move(builderRef);

                    --numRemainingThreads;
                    doneSignal.release();
                }));
        }

        auto startTime = AZStd::chrono::monotonic_clock::now();

        while (numRemainingThreads > 0)
        {
            doneSignal.try_acquire_for(AZStd::chrono::milliseconds(WaitIntervalMs));
        }

        auto endTime = AZStd::chrono::monotonic_clock::now();
        auto duration = endTime - startTime;

        for (auto&& thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        ASSERT_LT(duration, AZStd::chrono::seconds(WaitIntervalMs));

        ASSERT_EQ(builderManager.GetBuilderCreationCount(), NumberOfThreads + 1); // +1 because the manager starts one up to begin with
    }
}
