/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/TestImpactTestRunner.h>
#include <TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h>
#include <TestRunner/Native/Shard/TestImpactNativeShardedTestJob.h>
#include <TestRunner/Native/Shard/TestImpactNativeShardedTestSystemBus.h>

#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/utility/to_underlying.h>

namespace TestImpact
{
    //! 
    template<typename TestRunnerType>
    class NativeShardedTestSystem
    {
    public:
        using ShardedTestJobInfoType = ShardedTestJobInfo<TestRunnerType>;

        //! Constructs the sharded test system to wrap around the specified test runner.
        NativeShardedTestSystem(TestRunnerType& testRunner);

        //! Wrapper around the test runner's `RunTests` method to present the sharded test running interface to the user.
        [[nodiscard]] AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename TestRunnerType::Job>> RunTests(
            const AZStd::vector<typename ShardedTestJobInfoType>& shardedJobInfos,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout);

    private:
        //!
        class TestJobRunnerNotificationHandler;

        TestRunnerType* m_testRunner = nullptr; //!<
    };

    template<typename TestRunnerType>
    class NativeShardedTestSystem<TestRunnerType>::TestJobRunnerNotificationHandler
        : private TestRunnerType::NotificationsBus::Handler
    {
    public:
        using JobInfo = typename TestRunnerType::JobInfo;

        TestJobRunnerNotificationHandler(
            AZStd::unordered_map<JobInfo, const ShardedTestJobInfoType*>& shardToParentShardedJobMap,
            AZStd::unordered_map<const ShardedTestJobInfoType*, ShardedTestJob<TestRunnerType>>& completedShardMap);
        virtual ~TestJobRunnerNotificationHandler();

    private:
        // NotificationsBus overrides ...
        ProcessCallbackResult OnJobComplete(
            const typename TestRunnerType::Job::Info& jobInfo, const JobMeta& meta, const StdContent& std) override;

        AZStd::unordered_map<JobInfo, const ShardedTestJobInfoType*>* m_shardToParentShardedJobMap = nullptr;
        AZStd::unordered_map<const ShardedTestJobInfoType*, ShardedTestJob<TestRunnerType>>* m_completedShardMap = nullptr;
    };

    template<typename TestRunnerType>
    NativeShardedTestSystem<TestRunnerType>::TestJobRunnerNotificationHandler::TestJobRunnerNotificationHandler(
        AZStd::unordered_map<JobInfo, const ShardedTestJobInfoType*>& shardToParentShardedJobMap,
        AZStd::unordered_map<const ShardedTestJobInfoType*, ShardedTestJob<TestRunnerType>>& completedShardMap)
        : m_shardToParentShardedJobMap(&shardToParentShardedJobMap)
        , m_completedShardMap(&completedShardMap)
    {
        TestRunnerType::NotificationsBus::Handler::BusConnect();
    }

    template<typename TestRunnerType>
    NativeShardedTestSystem<TestRunnerType>::TestJobRunnerNotificationHandler::~TestJobRunnerNotificationHandler()
    {
        TestRunnerType::NotificationsBus::Handler::BusDisconnect();
    }

    template<typename TestRunnerType>
    ProcessCallbackResult NativeShardedTestSystem<TestRunnerType>::TestJobRunnerNotificationHandler::OnJobComplete(
        const typename TestRunnerType::Job::Info& jobInfo, const JobMeta& meta, const StdContent& std)
    {
        const auto& shardedJobInfo = m_shardToParentShardedJobMap->at(jobInfo);
        auto& shardedTestJob = m_completedShardMap->at(shardedJobInfo);
        
        {
            AZ::EBusAggregateResults<ProcessCallbackResult> results;
            NativeShardedTestSystemNotificationsBus<TestRunnerType>::BroadcastResult(
                results,
                &NativeShardedTestSystemNotificationsBus<TestRunnerType>::Events::OnShardedSubJobComplete,
                shardedJobInfo->second.begin()->GetId(),
                shardedJobInfo->second.size(),
                jobInfo,
                meta,
                std);

            const auto result = GetAggregateProcessCallbackResult(results);
            if (result == ProcessCallbackResult::Abort)
            {
                return result;
            }
        }

        shardedTestJob.RegisterCompletedSubJob(jobInfo, meta, std);

        if (shardedTestJob.IsComplete())
        {
            auto& consolidatedJobData = *shardedTestJob.GetConsolidatedJobData();
            AZ::EBusAggregateResults<ProcessCallbackResult> results;
            NativeShardedTestSystemNotificationsBus<TestRunnerType>::BroadcastResult(
                results,
                &NativeShardedTestSystemNotificationsBus<TestRunnerType>::Events::OnShardedJobComplete,
                consolidatedJobData.m_jobInfo,
                consolidatedJobData.m_meta,
                consolidatedJobData.m_std);
            return GetAggregateProcessCallbackResult(results);
        }

        return ProcessCallbackResult::Continue;
    }

    template<typename TestRunnerType>
    NativeShardedTestSystem<TestRunnerType>::NativeShardedTestSystem(TestRunnerType& testRunner)
        : m_testRunner(&testRunner)
    {
    }

    template<typename TestRunnerType>
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename TestRunnerType::Job>> NativeShardedTestSystem<TestRunnerType>::RunTests(
        const AZStd::vector<ShardedTestJobInfoType>& shardedJobInfos,
        StdOutputRouting stdOutRouting,
        StdErrorRouting stdErrRouting,
        AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout)
    {
        using JobInfo = typename TestRunnerType::JobInfo;
        using JobId = typename JobInfo::Id;

        // Key: sub-job shard
        // Value: parent sharded job info
        AZStd::unordered_map<JobInfo, const ShardedTestJobInfoType*> shardToParentShardedJobMap;
        AZStd::unordered_map<const ShardedTestJobInfoType*, ShardedTestJob<TestRunnerType>> completedShardMap;

        const auto totalJobShards = AZStd::accumulate(
            shardedJobInfos.begin(),
            shardedJobInfos.end(),
            size_t{ 0 },
            [](size_t sum, const ShardedTestJobInfoType& shardedJobInfo)
            {
                return sum + shardedJobInfo.second.size();
            });

        AZStd::vector<JobInfo> subJobInfos;
        subJobInfos.reserve(totalJobShards);
        for (const auto& shardedJobInfo : shardedJobInfos)
        {
            completedShardMap.emplace(
                AZStd::piecewise_construct, AZStd::forward_as_tuple(&shardedJobInfo), std::forward_as_tuple(shardedJobInfo));
            const auto& [testTarget, jobInfos] = shardedJobInfo;
            subJobInfos.insert(subJobInfos.end(), jobInfos.begin(), jobInfos.end());
            for (const auto& jobInfo : jobInfos)
            {
                shardToParentShardedJobMap[jobInfo] = &shardedJobInfo;
            }
        }

        TestJobRunnerNotificationHandler handler(shardToParentShardedJobMap, completedShardMap);
        const auto result = m_testRunner->RunTests(
            subJobInfos,
            stdOutRouting,
            stdErrRouting,
            runTimeout,
            runnerTimeout);

        AZStd::vector<typename TestRunnerType::Job> consilidatedJobs;
        consilidatedJobs.reserve(shardedJobInfos.size());


        return {};
    }
} // namespace TestImpact
