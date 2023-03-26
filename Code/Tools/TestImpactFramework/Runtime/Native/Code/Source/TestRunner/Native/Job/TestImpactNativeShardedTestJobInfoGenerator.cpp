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
        const NativeShardedArtifactDir& artifactDir,
        const RepoPath& testRunnerBinary,
        const RepoPath& instrumentBinary,
        CoverageLevel coverageLevel)
        : NativeShardedTestRunJobInfoGeneratorBase<NativeInstrumentedTestRunner>(
            maxConcurrency,
            sourceDir,
            targetBinaryDir,
            artifactDir,
            testRunnerBinary)
        , m_instrumentBinary(instrumentBinary)
        , m_coverageLevel(coverageLevel)
    {
    }

    ShardedInstrumentedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::GenerateJobInfo(
        const TestTargetAndEnumeration& testTargetAndEnumeration,
        typename NativeInstrumentedTestRunner::JobInfo::Id startingId) const
    {
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        const auto launchArgument = GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary);
        const auto testFilters = TestListsToTestFilters(ShardTestInterleaved(testTargetAndEnumeration));
        ShardedInstrumentedTestJobInfo shards(testTarget, typename ShardedInstrumentedTestJobInfo::second_type());
        shards.second.reserve(testFilters.size());

        for (size_t i = 0; i < testFilters.size(); i++)
        {
            const auto shardedRunArtifact = AZStd::string::format(
                "%s.%zu", GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_shardedTestRunArtifactDirectory).c_str(), i);

            const auto shardCoverageArtifact = AZStd::string::format(
                "%s.%zu", GenerateTargetCoverageArtifactFilePath(testTarget, m_artifactDir.m_shardedCoverageArtifactDirectory).c_str(), i);

            const RepoPath shardAdditionalArgsFile = AZStd::string::format("%s.args.%zu", (m_artifactDir.m_shardedTestRunArtifactDirectory / RepoPath(testTarget->GetName())).c_str(), i);

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

    ShardedRegularTestJobInfo NativeShardedRegularTestRunJobInfoGenerator::GenerateJobInfo(
        const TestTargetAndEnumeration& testTargetAndEnumeration,
        typename NativeRegularTestRunner::JobInfo::Id startingId) const
    {
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        const auto launchArgument = GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary);
        const auto testFilters = TestListsToTestFilters(ShardTestInterleaved(testTargetAndEnumeration));
        ShardedRegularTestJobInfo shards(testTarget, typename ShardedRegularTestJobInfo::second_type());
        shards.second.reserve(testFilters.size());

        for (size_t i = 0; i < testFilters.size(); i++)
        {
            const auto shardedRunArtifact = AZStd::string::format(
                "%s.%zu", GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_shardedTestRunArtifactDirectory).c_str(), i);

            const RepoPath shardAdditionalArgsFile = AZStd::string::format(
                "%s.args.%zu", (m_artifactDir.m_shardedTestRunArtifactDirectory / RepoPath(testTarget->GetName())).c_str(), i);

            const auto shardLaunchCommand =
                AZStd::string::format("%s --args_from_file \"%s\"", launchArgument.c_str(), shardAdditionalArgsFile.c_str());

            WriteFileContents<TestRunnerException>(testFilters[i], shardAdditionalArgsFile);

            const auto command = GenerateRegularTestJobInfoCommand(shardLaunchCommand, shardedRunArtifact);

            shards.second.emplace_back(
                NativeRegularTestRunner::JobInfo::Id{ startingId.m_value + i },
                command,
                NativeRegularTestRunner::JobData(testTarget->GetLaunchMethod(), shardedRunArtifact));
        }

        return shards;
    }
} // namespace TestImpact
