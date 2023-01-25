/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

#include <TestEngine/Common/Enumeration/TestImpactTestEngineEnumeration.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>

namespace TestImpact
{
    class NativeShardedInstrumentedTestRunJobInfoGenerator
    {
    public:
        NativeShardedInstrumentedTestRunJobInfoGenerator(
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const ArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary,
            CoverageLevel coverageLevel = CoverageLevel::Source);

        typename NativeInstrumentedTestRunner::JobInfo GenerateJobInfo(const TestEngineEnumeration& enumeration)
    };
} // namespace TestImpact
