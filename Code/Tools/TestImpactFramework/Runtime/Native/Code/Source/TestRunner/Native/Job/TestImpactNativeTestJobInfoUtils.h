/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRepoPath.h>

#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>

namespace TestImpact
{
    class NativeTestTarget;

    //! Generates the command string to launch the specified test target.
    AZStd::string GenerateLaunchArgument(
        const NativeTestTarget* testTarget, const RepoPath& targetBinaryDir, const RepoPath& testRunnerBinary);

    typename NativeRegularTestRunner::Command GenerateRegularTestJobInfoCommand();

    typename NativeInstrumentedTestRunner::Command GenerateInstrumentedTestJobInfoCommand(
        const RepoPath& instrumentBindaryPath,
        const RepoPath& coverageArtifactPath,
        CoverageLevel coverageLevel,
        const RepoPath& modulesPath,
        const RepoPath& excludedModulesPath,
        const RepoPath& sourcesPath,
        const typename NativeRegularTestRunner::Command& testRunLaunchCommand);
} // namespace TestImpact
