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
#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoUtils.h>

namespace TestImpact
{
    //! The CTest label that test target suites need to have in order to be sharded.
    inline constexpr auto TiafShardingLabel = "TIAF_sharding";

    //!
    template<typename TestJobRunner>
    using ShardedTestJobInfo = AZStd::pair<const NativeTestTarget*, AZStd::vector<typename TestJobRunner::JobInfo>>;

    //!
    using ShardedInstrumentedTestJobInfo = ShardedTestJobInfo<NativeInstrumentedTestRunner>;

    //!
    using ShardedRegularTestJobInfo = ShardedTestJobInfo<NativeRegularTestRunner>;

    using TestTargetAndEnumeration = AZStd::pair<const NativeTestTarget*, const TestEnumeration*>;

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
            const TestTargetAndEnumeration& testTargetAndEnumeration,
            typename TestJobRunner::JobInfo::Id startingId) const = 0;

        AZStd::vector<ShardedTestJobInfo<TestJobRunner>> GenerateJobInfos(
            const AZStd::vector<TestTargetAndEnumeration>& testTargetsAndEnumerations);

    protected:
        //!
        using ShardedTestsList = AZStd::vector<AZStd::vector<AZStd::string>>;

        //!
        using ShardedTestsFilter = AZStd::vector<AZStd::string>;

        //!
        ShardedTestsList ShardTestInterleaved(const TestTargetAndEnumeration& testTargetAndEnumeration) const;

        //!
        ShardedTestsFilter TestListsToTestFilters(const ShardedTestsList& shardedTestList) const;

        //!
        RepoPath GenerateShardedTargetRunArtifactFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

        //!
        RepoPath GenerateShardedAdditionalArgsFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

        //!
        AZStd::string GenerateShardedLaunchCommand(
            const NativeTestTarget* testTarget, const RepoPath& shardAdditionalArgsFile) const;

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
            const TestTargetAndEnumeration& testTargetAndEnumeration,
            typename NativeInstrumentedTestRunner::JobInfo::Id startingId) const override;

    private:
        //!
        RepoPath GenerateShardedTargetCoverageArtifactFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

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
            const TestTargetAndEnumeration& testTargetAndEnumeration,
            typename NativeRegularTestRunner::JobInfo::Id startingId) const override;
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

    template<typename TestJobRunner>
    typename NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::ShardedTestsList NativeShardedTestRunJobInfoGeneratorBase<
        TestJobRunner>::ShardTestInterleaved(const TestTargetAndEnumeration& testTargetAndEnumeration) const
    {
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        if (testTarget->GetSuiteLabelSet().contains(TiafShardingLabel) && testEnumeration)
        {
            const auto numTests = testEnumeration->GetNumEnabledTests();
            const auto numShards = std::min(m_maxConcurrency, numTests);
            ShardedTestsList shardTestList(numShards);
            const auto testsPerShard = numTests / numShards;

            size_t testIndex = 0;
            for (const auto fixture : testEnumeration->GetTestSuites())
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
        else
        {
            ShardedTestsList shardTestList(1);
            shardTestList[0].emplace_back("*.*");
            return shardTestList;
        }
    }

    template<typename TestJobRunner>
    auto NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::TestListsToTestFilters(const ShardedTestsList& shardedTestList) const
        -> ShardedTestsFilter
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

    template<typename TestJobRunner>
    auto NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateJobInfos(
        const AZStd::vector<TestTargetAndEnumeration>& testTargetsAndEnumerations)
        -> AZStd::vector<ShardedTestJobInfo<TestJobRunner>>
    {
        AZStd::vector<ShardedTestJobInfo<TestJobRunner>> jobInfos;
        for (size_t testTargetIndex = 0, jobId = 0; testTargetIndex < testTargetsAndEnumerations.size(); testTargetIndex++)
        {
            jobInfos.push_back(GenerateJobInfo(testTargetsAndEnumerations[testTargetIndex], { jobId }));
            jobId += jobInfos.back().second.size();
        }

        return jobInfos;
    }

    template<typename TestJobRunner>
    RepoPath NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateShardedTargetRunArtifactFilePath(
        const NativeTestTarget* testTarget, const size_t shardNumber) const
    {
        auto artifactFilePath = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_shardedTestRunArtifactDirectory);
        return artifactFilePath.ReplaceExtension(AZStd::string::format("%zu%s", shardNumber, artifactFilePath.Extension().String().c_str()).c_str());
    }

    template<typename TestJobRunner>
    AZStd::string NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateShardedLaunchCommand(
        const NativeTestTarget* testTarget, const RepoPath& shardAdditionalArgsFile) const
    {
        return AZStd::string::format(
            "%s --args_from_file \"%s\"",
            GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary).c_str(),
            shardAdditionalArgsFile.c_str());
    }

    template<typename TestJobRunner>
    RepoPath NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateShardedAdditionalArgsFilePath(
        const NativeTestTarget* testTarget, size_t shardNumber) const
    {
        return AZStd::string::format(
            "%s.%zu.args", (m_artifactDir.m_shardedTestRunArtifactDirectory / RepoPath(testTarget->GetName())).c_str(), shardNumber);
    }
} // namespace TestImpact
