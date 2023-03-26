/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/Native/TestImpactNativeConfiguration.h>

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/Common/Enumeration/TestImpactTestEngineEnumeration.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>

namespace TestImpact
{
    //!
    template<typename TestJobRunner>
    using ShardedTestJobInfo = AZStd::pair<const NativeTestTarget*, AZStd::vector<typename TestJobRunner::JobInfo>>;

    //!
    using ShardedInstrumentedTestJobInfo = ShardedTestJobInfo<NativeInstrumentedTestRunner>;

    //!
    using ShardedRegularTestJobInfo = ShardedTestJobInfo<NativeRegularTestRunner>;

    //!
    template<typename TestJobRunner>
    class NativeShardedTestRunJobInfoGeneratorBase
    {
    public:
        NativeShardedTestRunJobInfoGeneratorBase(
            size_t maxConcurrency,
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const NativeShardedArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary);

        virtual ~NativeShardedTestRunJobInfoGeneratorBase() = default;

        virtual ShardedTestJobInfo<TestJobRunner> GenerateJobInfo(
            const NativeTestTarget* testTarget,
            const TestEnumeration& enumeration,
            typename TestJobRunner::JobInfo::Id startingId) = 0;

    protected:
        //!
        using ShardedTestsList = AZStd::vector<AZStd::vector<AZStd::string>>;

        //!
        using ShardedTestsFilter = AZStd::vector<AZStd::string>;

        ShardedTestsList ShardFixtureContiguous(const TestEnumeration& enumeration);
        ShardedTestsList ShardTestContiguous(const TestEnumeration& enumeration);
        ShardedTestsList ShardFixtureInterleaved(const TestEnumeration& enumeration);
        ShardedTestsList ShardTestInterleaved(const TestEnumeration& enumeration);
        ShardedTestsFilter TestListsToTestFilters(const ShardedTestsList& shardedTestList);

        size_t m_maxConcurrency;
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        NativeShardedArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;
    };

    //!
    class NativeShardedInstrumentedTestRunJobInfoGenerator
        : public NativeShardedTestRunJobInfoGeneratorBase<NativeInstrumentedTestRunner>
    {
    public:
        NativeShardedInstrumentedTestRunJobInfoGenerator(
            size_t maxConcurrency,
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const NativeShardedArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary,
            CoverageLevel coverageLevel = CoverageLevel::Source);

        //!
        ShardedInstrumentedTestJobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget,
            const TestEnumeration& enumeration,
            typename NativeInstrumentedTestRunner::JobInfo::Id startingId) override;

    private:
        RepoPath m_cacheDir;
        RepoPath m_instrumentBinary;
        CoverageLevel m_coverageLevel;
    };

    //!
    class NativeShardedRegularTestRunJobInfoGenerator
        :  public NativeShardedTestRunJobInfoGeneratorBase<NativeRegularTestRunner>
    {
    public:
        using NativeShardedTestRunJobInfoGeneratorBase<NativeRegularTestRunner>::NativeShardedTestRunJobInfoGeneratorBase;

        //!
        ShardedRegularTestJobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget,
            const TestEnumeration& enumeration,
            typename NativeRegularTestRunner::JobInfo::Id startingId) override;
    };

    template<typename TestJobRunner>
    NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::NativeShardedTestRunJobInfoGeneratorBase(
        size_t maxConcurrency,
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        const NativeShardedArtifactDir& artifactDir,
        const RepoPath& testRunnerBinary)
        : m_maxConcurrency(maxConcurrency)
        , m_sourceDir(sourceDir)
        , m_targetBinaryDir(targetBinaryDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
    {
        AZ_TestImpact_Eval(maxConcurrency != 0, TestRunnerException, "Max Number of concurrent processes in flight cannot be 0");
    }

    // InstrumentedShardedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::ShardFixtureContiguous(const
    // TestEngineEnumeration<NativeTestTarget>& enumeration)
    //{
    //
    // }
    //
    // InstrumentedShardedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::ShardTestContiguous(
    //     const TestEngineEnumeration<NativeTestTarget>& enumeration)
    //{
    // }
    //
    // InstrumentedShardedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::ShardFixtureInterleaved(
    //     const TestEngineEnumeration<NativeTestTarget>& enumeration)
    //{
    // }
    //
    template<typename TestJobRunner>
    typename NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::ShardedTestsList NativeShardedTestRunJobInfoGeneratorBase<
        TestJobRunner>::ShardTestInterleaved(const TestEnumeration& enumeration)
    {
        const auto numTests = enumeration.GetNumEnabledTests();
        const auto numShards = std::min(m_maxConcurrency, numTests);
        ShardedTestsList shardTestList(numShards);
        const auto testsPerShard = numTests / numShards;

        size_t testIndex = 0;
        for (const auto fixture : enumeration.GetTestSuites())
        {
            if (!fixture.m_enabled)
            {
                continue;
            }

            for (const auto test : fixture.m_tests)
            {
                if (!test.m_enabled)
                {
                    continue;
                }

                shardTestList[testIndex++ % numShards].emplace_back(
                    AZStd::string::format("%s.%s", fixture.m_name.c_str(), test.m_name.c_str()));
            }
        }

        return shardTestList;
    }

    template<typename TestJobRunner>
    typename NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::ShardedTestsFilter NativeShardedTestRunJobInfoGeneratorBase<
        TestJobRunner>::TestListsToTestFilters(const ShardedTestsList& shardedTestList)
    {
        ShardedTestsFilter shardedTestFilter;
        shardedTestFilter.reserve(shardedTestList.size());

        for (const auto& shardTests : shardedTestList)
        {
            AZStd::string testFilter = "--gtest_filter=";
            for (const auto& test : shardTests)
            {
                // The trailing colon added by the last test is still a valid gtest filter
                testFilter += AZStd::string::format("%s:", test.c_str());
            }

            shardedTestFilter.emplace_back(testFilter);
        }

        return shardedTestFilter;
    }
} // namespace TestImpact
