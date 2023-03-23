/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
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
        using Job = typename TestRunnerType::Job;
        using JobInfo = typename TestRunnerType::JobInfo;
        using JobId = typename JobInfo::Id;
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
        template<typename TestRunnerT>
        using ShardToParentShardedJobMap = AZStd::unordered_map<typename JobId::IdType, const ShardedTestJobInfo<TestRunnerT>*>;

        //!
        template<typename TestRunnerT>
        using CompletedShardMap = AZStd::unordered_map<const ShardedTestJobInfo<TestRunnerT>*, ShardedTestJob<TestRunnerT>>;

        //!
        static [[nodiscard]] typename NativeRegularTestRunner::ResultType ConsolidateSubJobs(
            const typename NativeRegularTestRunner::ResultType& result,
            const ShardToParentShardedJobMap<NativeRegularTestRunner>& shardToParentShardedJobMap,
            const CompletedShardMap<NativeRegularTestRunner>& completedShardMap);

        //!
        static [[nodiscard]] typename NativeInstrumentedTestRunner::ResultType ConsolidateSubJobs(
            const typename NativeInstrumentedTestRunner::ResultType& result,
            const ShardToParentShardedJobMap<NativeInstrumentedTestRunner>& shardToParentShardedJobMap,
            const CompletedShardMap<NativeInstrumentedTestRunner>& completedShardMap);

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

        TestJobRunnerNotificationHandler(ShardToParentShardedJobMap<TestRunnerType>& shardToParentShardedJobMap, CompletedShardMap<TestRunnerType>& completedShardMap);
        virtual ~TestJobRunnerNotificationHandler();

    private:
        // NotificationsBus overrides ...
        ProcessCallbackResult OnJobComplete(
            const typename TestRunnerType::Job::Info& jobInfo, const JobMeta& meta, const StdContent& std) override;

        ShardToParentShardedJobMap<TestRunnerType>* m_shardToParentShardedJobMap = nullptr;
        CompletedShardMap<TestRunnerType>* m_completedShardMap = nullptr;
    };

    template<typename TestRunnerType>
    NativeShardedTestSystem<TestRunnerType>::TestJobRunnerNotificationHandler::TestJobRunnerNotificationHandler(
        ShardToParentShardedJobMap<TestRunnerType>& shardToParentShardedJobMap, CompletedShardMap<TestRunnerType>& completedShardMap)
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
        const auto& shardedJobInfo = m_shardToParentShardedJobMap->at(jobInfo.GetId().m_value);
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
    typename NativeRegularTestRunner::ResultType NativeShardedTestSystem<TestRunnerType>::ConsolidateSubJobs(
        [[maybe_unused]] const typename NativeRegularTestRunner::ResultType& result,
        [[maybe_unused]] const ShardToParentShardedJobMap<NativeRegularTestRunner>& shardToParentShardedJobMap,
        [[maybe_unused]] const CompletedShardMap<NativeRegularTestRunner>& completedShardMap)
    {
        return {};
    }

    template<typename TestRunnerType>
    typename NativeInstrumentedTestRunner::ResultType NativeShardedTestSystem<TestRunnerType>::ConsolidateSubJobs(
        const typename NativeInstrumentedTestRunner::ResultType& result,
        const ShardToParentShardedJobMap<NativeInstrumentedTestRunner>& shardToParentShardedJobMap,
        const CompletedShardMap<NativeInstrumentedTestRunner>& completedShardMap)
    {
        const auto& [returnCode, subJobs] = result;
        AZStd::unordered_map<typename JobId::IdType, std::pair<AZStd::unordered_map<AZStd::string, TestRunSuite>, AZStd::unordered_map<RepoPath, ModuleCoverage>>>
            consolidatedJobArtifacts;

        for (const auto& subJob : subJobs)
        {
            if (const auto payload = subJob.GetPayload();
                payload.has_value())
            {
                const auto& [subTestRun, subTestCoverage] = payload.value();
                const auto shardedTestJobInfo = shardToParentShardedJobMap.at(subJob.GetJobInfo().GetId().m_value);

                // The parent job info id of the sharded sub jobs is the id of the first sub job info
                const auto parentJobInfoId = shardedTestJobInfo->second.front().GetId();
                auto& [testSuites, testCoverage] = consolidatedJobArtifacts[parentJobInfoId.m_value];

                // Accumulate test results
                if (subTestRun.has_value())
                {
                    for (const auto& subTestSuite : subTestRun->GetTestSuites())
                    {
                        auto& testSuite = testSuites[subTestSuite.m_name];
                        if (testSuite.m_name.empty())
                        {
                            testSuite.m_name = subTestSuite.m_name;
                        }

                        testSuite.m_enabled = subTestSuite.m_enabled;
                        testSuite.m_duration += subTestSuite.m_duration;
                        testSuite.m_tests.insert(testSuite.m_tests.end(), subTestSuite.m_tests.begin(), subTestSuite.m_tests.end());
                    }
                }

                // Accumulate test coverage
                for (const auto& subModuleCoverage : subTestCoverage.GetModuleCoverages())
                {
                    auto& moduleCoverage = testCoverage[subModuleCoverage.m_path];
                    if (moduleCoverage.m_path.empty())
                    {
                        moduleCoverage.m_path = subModuleCoverage.m_path;
                        moduleCoverage.m_sources.insert(
                            moduleCoverage.m_sources.end(), subModuleCoverage.m_sources.begin(), subModuleCoverage.m_sources.end());
                    }
                }
            }
        }

        AZStd::vector<typename TestRunnerType::Job> consolidatedJobs;
        consolidatedJobs.reserve(consolidatedJobArtifacts.size());

        for (auto&& [jobId, artifacts] : consolidatedJobArtifacts)
        {
            AZStd::optional<TestRun> run;
            auto&& [testSuites, testCoverage] = artifacts;
            const auto shardedTestJobInfo = shardToParentShardedJobMap.at(jobId);
            const auto& shardedTestJob = completedShardMap.at(shardedTestJobInfo);
            const auto& jobData = shardedTestJob.GetConsolidatedJobData();
            if (testSuites.size())
            {
                AZStd::vector<TestRunSuite> suites;
                suites.reserve(testSuites.size());
                for (auto&& [suiteName, suite] : testSuites)
                {
                    suites.emplace_back(AZStd::move(suite));
                }

                if (jobData.has_value())
                {
                    run = TestRun(AZStd::move(suites), jobData->m_meta.m_duration.value_or(AZStd::chrono::milliseconds{ 0 }));
                }
            }

            AZStd::vector<ModuleCoverage> moduleCoverages;
            moduleCoverages.reserve(testCoverage.size());
            for (auto&& [modulePath, moduleCoverage] : testCoverage)
            {
                moduleCoverages.emplace_back(AZStd::move(moduleCoverage));
            }

            auto payload = typename TestRunnerType::JobPayload{ run, TestCoverage(AZStd::move(moduleCoverages)) };
            consolidatedJobs.emplace_back(jobData->m_jobInfo, JobMeta{ jobData->m_meta }, AZStd::move(payload));
        }

        return { result.first, consolidatedJobs };
    }

    template<typename TestRunnerType>
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename TestRunnerType::Job>> NativeShardedTestSystem<TestRunnerType>::RunTests(
        const AZStd::vector<ShardedTestJobInfoType>& shardedJobInfos,
        StdOutputRouting stdOutRouting,
        StdErrorRouting stdErrRouting,
        AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout)
    {
        // Key: sub-job shard
        // Value: parent sharded job info
        ShardToParentShardedJobMap<TestRunnerType> shardToParentShardedJobMap;
        CompletedShardMap<TestRunnerType> completedShardMap;

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
                shardToParentShardedJobMap[jobInfo.GetId().m_value] = &shardedJobInfo;
            }
        }

        TestJobRunnerNotificationHandler handler(shardToParentShardedJobMap, completedShardMap);
        const auto result = m_testRunner->RunTests(
            subJobInfos,
            stdOutRouting,
            stdErrRouting,
            runTimeout,
            runnerTimeout);

         return ConsolidateSubJobs(result, shardToParentShardedJobMap, completedShardMap);
    }
} // namespace TestImpact
