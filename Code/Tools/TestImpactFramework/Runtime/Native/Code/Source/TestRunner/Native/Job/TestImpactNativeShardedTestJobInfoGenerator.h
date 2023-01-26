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

namespace TestImpact
{
    //!
    template<typename TestJobRunner>
    using ShardedTestJobInfo = AZStd::pair<const NativeTestTarget*, AZStd::vector<typename TestJobRunner::JobInfo>>;

    //!
    using InstrumentedShardedTestJobInfo = ShardedTestJobInfo<NativeInstrumentedTestRunner>;

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
            const TestEngineEnumeration<NativeTestTarget>& enumeration);

        //!
        AZStd::vector<InstrumentedShardedTestJobInfo> GenerateJobInfos(
            AZStd::vector<TestEngineEnumeration<NativeTestTarget>>& testTargets) const;

    private:
        InstrumentedShardedTestJobInfo ShardFixtureContiguous(const TestEngineEnumeration<NativeTestTarget>& enumeration);
        InstrumentedShardedTestJobInfo ShardTestContiguous(const TestEngineEnumeration<NativeTestTarget>& enumeration);
        InstrumentedShardedTestJobInfo ShardFixtureInterleaved(const TestEngineEnumeration<NativeTestTarget>& enumeration);
        InstrumentedShardedTestJobInfo ShardTestInterleaved(const TestEngineEnumeration<NativeTestTarget>& enumeration);

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
