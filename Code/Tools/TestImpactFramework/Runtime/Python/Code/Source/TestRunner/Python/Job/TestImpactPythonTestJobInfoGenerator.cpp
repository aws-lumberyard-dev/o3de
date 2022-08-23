/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestRunner/Python/Job/TestImpactPythonTestJobInfoGenerator.h>

#include <AzCore/StringFunc/StringFunc.h>

namespace TestImpact
{

    PythonTestRunJobInfoGenerator::PythonTestRunJobInfoGenerator(
        const RepoPath& repoDir, const RepoPath& buildDir, const ArtifactDir& artifactDir)
        : m_repoDir(repoDir)
        , m_buildDir(buildDir)
        , m_artifactDir(artifactDir)
    {
    }

    AZStd::string CompileParentFolderName(const PythonTestTarget* testTarget)
    {
        // Compile a unique folder name based on the aprent script path
        auto parentfolder = testTarget->GetScriptPath().String();
        AZ::StringFunc::Replace(parentfolder, '/', '_');
        AZ::StringFunc::Replace(parentfolder, '\\', '_');
        AZ::StringFunc::Replace(parentfolder, '.', '_');
        return parentfolder;
    }

    PythonTestRunnerBase::JobInfo PythonTestRunJobInfoGenerator::GenerateJobInfo(
        const PythonTestTarget* testTarget, PythonTestRunnerBase::JobInfo::Id jobId) const
    {
        const auto parentFolderName = RepoPath(CompileParentFolderName(testTarget));
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory);
        const Command args = { testTarget->GetCommand() };

        return JobInfo(jobId, args, JobData(runArtifact, m_artifactDir.m_coverageArtifactDirectory / parentFolderName));
    }

    PythonTestEnumerationJobInfoGenerator::PythonTestEnumerationJobInfoGenerator(
        const RepoPath& buildDir, const RepoPath& cacheDir, const ArtifactDir& artifactDir, const RepoPath& pythonDir)
        : m_buildDir(buildDir)
        , m_cacheDir(cacheDir)
        , m_artifactDir(artifactDir)
        , m_pythonCommand(pythonDir)  
    {
    }

    void PythonTestEnumerationJobInfoGenerator::SetCachePolicy(PythonTestEnumerator::JobInfo::CachePolicy cachePolicy)
    {
        m_cachePolicy = cachePolicy;
    }

    PythonTestEnumerator::JobInfo::CachePolicy PythonTestEnumerationJobInfoGenerator::GetCachePolicy() const
    {
        return m_cachePolicy;
    }

    AZStd::string PythonTestEnumerationJobInfoGenerator::CompileBuildDirectoryArgument() const
    {
        return AZStd::string::format("--build-directory=%s ", m_buildDir.c_str());
    }

    PythonTestEnumerator::JobInfo PythonTestEnumerationJobInfoGenerator::GenerateJobInfo(
        const PythonTestTarget* testTarget, PythonTestEnumerator::JobInfo::Id jobId) const
    {
        using Cache = PythonTestEnumerator::JobData::Cache;

        const auto enumerationArtifact = GenerateTargetEnumerationArtifactFilePath(testTarget, m_artifactDir.m_enumerationArtifactDirectory);
        const AZStd::string pytestAndFixedArgs = " -m pytest --collect-only --continue-on-collection-errors -q ";
        const AZStd::string buildDirectory = CompileBuildDirectoryArgument();
        const AZStd::string scriptPath = testTarget->GetScriptPath().String();
        const Command args = { m_pythonCommand.String() + pytestAndFixedArgs + buildDirectory + scriptPath + " >" + enumerationArtifact.String()};

        return JobInfo(jobId, args, JobData(enumerationArtifact, Cache{ m_cachePolicy, GenerateTargetEnumerationCacheFilePath(testTarget, m_cacheDir)}));
    }

} // namespace TestImpact
