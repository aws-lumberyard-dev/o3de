/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>

#include <Target/Python/TestImpactPythonTestTarget.h>
#include <TestRunner/Common/Job/TestImpactTestJobInfoGenerator.h>
#include <TestRunner/Python/TestImpactPythonTestRunner.h>
#include <TestRunner/Python/TestImpactPythonTestEnumerator.h>

namespace TestImpact
{
    //! Generates job information for the test job runner.
    class PythonTestRunJobInfoGenerator
        : public TestJobInfoGenerator<PythonTestRunnerBase, PythonTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param repoDir Root path to where the repository is located.
        //! @param buildDir Path to where the target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        PythonTestRunJobInfoGenerator(
            const RepoPath& repoDir, const RepoPath& buildDir, const ArtifactDir& artifactDir);

        //! Generates the information for a test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        PythonTestRunnerBase::JobInfo GenerateJobInfo(const PythonTestTarget* testTarget, PythonTestRunnerBase::JobInfo::Id jobId) const;

    private:
        RepoPath m_repoDir;
        RepoPath m_buildDir;
        ArtifactDir m_artifactDir;
    };

    class PythonTestEnumerationJobInfoGenerator
        : public TestJobInfoGenerator<PythonTestEnumerator, PythonTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param repoDir Root path to where the repository is located.
        //! @param buildDir Path to where the target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        PythonTestEnumerationJobInfoGenerator(
            const RepoPath& buildDir, const RepoPath& cacheDir, const ArtifactDir& artifactDir, const RepoPath& pythonDir);

        //! Generates the information for a test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        PythonTestEnumerator::JobInfo GenerateJobInfo(const PythonTestTarget* testTarget, PythonTestEnumerator::JobInfo::Id jobId) const;

        AZStd::string CompileBuildDirectoryArgument() const;

        void SetCachePolicy(PythonTestEnumerator::JobInfo::CachePolicy cachePolicy);

        //!
        PythonTestEnumerator::JobInfo::CachePolicy GetCachePolicy() const;
    private:
        RepoPath m_buildDir;
        RepoPath m_cacheDir;
        ArtifactDir m_artifactDir;
        RepoPath m_pythonCommand;

        PythonTestEnumerator::JobInfo::CachePolicy m_cachePolicy = PythonTestEnumerator::JobInfo::CachePolicy::Write;
    };
} // namespace TestImpact
