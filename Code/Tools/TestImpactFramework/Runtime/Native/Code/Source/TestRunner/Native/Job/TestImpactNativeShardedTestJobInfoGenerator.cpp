/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactTestSequence.h>

#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoUtils.h>

namespace TestImpact
{
    NativeShardedInstrumentedTestRunJobInfoGenerator::NativeShardedInstrumentedTestRunJobInfoGenerator(
        size_t maxConcurrency,
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        const ArtifactDir& artifactDir,
        const RepoPath& testRunnerBinary,
        const RepoPath& instrumentBinary,
        CoverageLevel coverageLevel)
        : m_maxConcurrency(maxConcurrency)
        , m_sourceDir(sourceDir)
        , m_targetBinaryDir(targetBinaryDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
        , m_instrumentBinary(instrumentBinary)
        , m_coverageLevel(coverageLevel)
    {
        AZ_TestImpact_Eval(maxConcurrency != 0, TestRunnerException, "Max Number of concurrent processes in flight cannot be 0");
    }
    
    //InstrumentedShardedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::ShardFixtureContiguous(const TestEngineEnumeration<NativeTestTarget>& enumeration)
    //{
    //
    //}
    //
    //InstrumentedShardedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::ShardTestContiguous(
    //    const TestEngineEnumeration<NativeTestTarget>& enumeration)
    //{
    //}
    //
    //InstrumentedShardedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::ShardFixtureInterleaved(
    //    const TestEngineEnumeration<NativeTestTarget>& enumeration)
    //{
    //}
    //
    typename NativeShardedInstrumentedTestRunJobInfoGenerator::ShardedTestsList NativeShardedInstrumentedTestRunJobInfoGenerator::
        ShardTestInterleaved(const TestEnumeration& enumeration)
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

                shardTestList[testIndex++ % numShards].emplace_back(AZStd::string::format("%s.%s", fixture.m_name.c_str(), test.m_name.c_str()));
            }
        }

        return shardTestList;
    }

    typename NativeShardedInstrumentedTestRunJobInfoGenerator::ShardedTestsFilter NativeShardedInstrumentedTestRunJobInfoGenerator::
        TestListsToTestFilters(
        const ShardedTestsList& shardedTestList)
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

    InstrumentedShardedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::GenerateJobInfo(
        const NativeTestTarget* testTarget,
        const TestEnumeration& enumeration,
        typename NativeInstrumentedTestRunner::JobInfo::Id startingId)
    {
        const auto launchArgument = GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary);
        const auto testFilters = TestListsToTestFilters(ShardTestInterleaved(enumeration));
        InstrumentedShardedTestJobInfo shards(testTarget, typename InstrumentedShardedTestJobInfo::second_type());
        shards.second.reserve(testFilters.size());

        for (size_t i = 0; i < testFilters.size(); i++)
        {
            const auto shardedRunArtifact = AZStd::string::format(
                "%s.%zu", GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory).c_str(), i);

            const auto shardCoverageArtifact = AZStd::string::format(
                "%s.%zu", GenerateTargetCoverageArtifactFilePath(testTarget, m_artifactDir.m_coverageArtifactDirectory).c_str(), i);

            const RepoPath shardAdditionalArgsFile = AZStd::string::format("%s.args.%zu", (m_artifactDir.m_testRunArtifactDirectory / RepoPath(testTarget->GetName())).c_str(), i);

            const auto shardLaunchCommand =
                AZStd::string::format("%s --args_from_file \"%s\"", launchArgument.c_str(), shardAdditionalArgsFile.c_str());

            WriteFileContents<TestRunnerException>(testFilters[i], shardAdditionalArgsFile);

            const auto command = GenerateInstrumentedTestJobInfoCommand(
                m_instrumentBinary,
                shardCoverageArtifact,
                m_coverageLevel,
                m_targetBinaryDir,
                m_testRunnerBinary,
                m_sourceDir,
                GenerateRegularTestJobInfoCommand(shardLaunchCommand, shardedRunArtifact));

            shards.second.emplace_back(
                NativeInstrumentedTestRunner::JobInfo::Id{ startingId.m_value + i },
                command,
                NativeInstrumentedTestRunner::JobData(testTarget->GetLaunchMethod(), shardedRunArtifact, shardCoverageArtifact));
        }

        return shards;
    }
} // namespace TestImpact
