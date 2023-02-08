/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

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
    using InstrumentedShardedTestJobInfo = ShardedTestJobInfo<NativeInstrumentedTestRunner>;

    //!
    using RegularShardedTestJobInfo = ShardedTestJobInfo<NativeRegularTestRunner>;

    //!
    class NativeShardedInstrumentedTestRunJobInfoGenerator
    {
    public:
        NativeShardedInstrumentedTestRunJobInfoGenerator(
            size_t maxConcurrency,
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const ArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary,
            CoverageLevel coverageLevel = CoverageLevel::Source);

        //!
        InstrumentedShardedTestJobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget, const TestEnumeration& enumeration, typename NativeInstrumentedTestRunner::JobInfo::Id startingId);

        //!
        //AZStd::vector<InstrumentedShardedTestJobInfo> GenerateJobInfos(
        //    AZStd::vector<TestEngineEnumeration<NativeTestTarget>>& testTargets) const;

    private:
        //!
        using ShardedTestsList = AZStd::vector<AZStd::vector<AZStd::string>>;

        //!
        using ShardedTestsFilter = AZStd::vector<AZStd::string>;

        ShardedTestsList ShardFixtureContiguous(const TestEnumeration& enumeration);
        ShardedTestsList ShardTestContiguous(const TestEnumeration& enumeration);
        ShardedTestsList ShardFixtureInterleaved(const TestEnumeration& enumeration);
        ShardedTestsList ShardTestInterleaved(const TestEnumeration& enumeration);
        ShardedTestsFilter TestListsToTestFilters(const ShardedTestsList& shardedTestList);

    private:
        size_t m_maxConcurrency;
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        RepoPath m_cacheDir;
        ArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;
        RepoPath m_instrumentBinary;
        CoverageLevel m_coverageLevel;
    };
} // namespace TestImpact
