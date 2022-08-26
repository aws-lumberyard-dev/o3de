/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Target/Python/TestImpactPythonTestTarget.h>
#include <TestEngine/Common/TestImpactTestEngineException.h>
#include <TestEngine/Python/TestImpactPythonErrorCodeChecker.h>
#include <TestEngine/Python/TestImpactPythonTestEngine.h>
#include <TestRunner/Python/Job/TestImpactPythonTestJobInfoGenerator.h>
#include <TestRunner/Python/TestImpactPythonTestRunner.h>
#include <TestRunner/Python/TestImpactPythonNullTestRunner.h>
#include <TestRunner/Python/TestImpactPythonTestEnumerator.h>

namespace TestImpact
{
    AZStd::optional<Client::TestRunResult> PythonInstrumentedTestRunnerErrorCodeChecker(
        [[maybe_unused]] const typename PythonTestRunner::JobInfo& jobInfo, const JobMeta& meta)
    {
        // The PyTest error code for test failures overlaps with the Python error code for script error so we have no way of
        // discerning at the job meta level whether a test failure or script execution error we will assume the tests failed for now
        if (auto result = CheckPyTestErrorCode(meta.m_returnCode.value()); result.has_value())
        {
            return result;
        }

        if (auto result = CheckPythonErrorCode(meta.m_returnCode.value()); result.has_value())
        {
            return result;
        }

        return AZStd::nullopt;
    }

    template<>
    struct TestJobRunnerTrait<PythonTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<typename PythonTestEngine::TestTargetType, typename PythonTestEngine::TestCaseCoverageType>;
    };

    template<>
    struct TestJobRunnerTrait<PythonNullTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<typename PythonTestEngine::TestTargetType, typename PythonTestEngine::TestCaseCoverageType>;
    };

    PythonTestEngine::PythonTestEngine(
        const RepoPath& repoDir,
        const RepoPath& buildDir,
        const RepoPath& cacheDir,
        const ArtifactDir& artifactDir,
        const RepoPath& pythonCommand,
        bool useNullTestRunner)
        : m_testRunJobInfoGenerator(AZStd::make_unique<PythonTestRunJobInfoGenerator>(
              repoDir, buildDir, artifactDir))
        , m_testEnumerationJobInfoGenerator(AZStd::make_unique<PythonTestEnumerationJobInfoGenerator>(repoDir, buildDir, cacheDir, artifactDir, pythonCommand))
        , m_testRunner(AZStd::make_unique<PythonTestRunner>(artifactDir))
        , m_nullTestRunner(AZStd::make_unique<PythonNullTestRunner>(artifactDir))
        , m_artifactDir(artifactDir)
        , m_useNullTestRunner(useNullTestRunner)
        , m_testEnumerator(AZStd::make_unique<PythonTestEnumerator>(32))
    {
    }

    PythonTestEngine::~PythonTestEngine() = default;

    void PythonTestEngine::DeleteArtifactXmls() const
    {
        DeleteFiles(m_artifactDir.m_testRunArtifactDirectory, "*.xml");
        DeleteFiles(m_artifactDir.m_coverageArtifactDirectory, "*.pycoverage");
    }

    TestEngineInstrumentedRunResult<typename PythonTestEngine::TestTargetType, typename PythonTestEngine::TestCaseCoverageType>
        PythonTestEngine::
        InstrumentedRun(
        [[maybe_unused]] const AZStd::vector<const PythonTestTarget*>& testTargets,
        [[maybe_unused]] Policy::ExecutionFailure executionFailurePolicy,
        [[maybe_unused]] Policy::IntegrityFailure integrityFailurePolicy,
        [[maybe_unused]] Policy::TestFailure testFailurePolicy,
        [[maybe_unused]] Policy::TargetOutputCapture targetOutputCapture,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]] AZStd::optional<TestEngineJobCompleteCallback<TestTargetType>> callback,
        [[maybe_unused]] AZStd::optional<TestEngineStdRoutingCallback<TestTargetType>> stdRoutingCallback) const
    {
        {
            const auto stdPrint = []([[maybe_unused]] const typename PythonTestEnumerator::JobInfo& jobInfo,
                                     [[maybe_unused]] const AZStd::string& stdOutput,
                                     [[maybe_unused]] const AZStd::string& stdError,
                                     AZStd::string&& stdOutDelta,
                                     [[maybe_unused]] AZStd::string&& stdErrDelta)
            {
                if (!stdOutDelta.empty())
                {
                    AZ_Printf("StdOut", stdOutDelta.c_str());
                }

                if (!stdErrDelta.empty())
                {
                    AZ_Printf("StdError", stdErrDelta.c_str());
                }

                return TestImpact::ProcessCallbackResult::Continue;
            };

            const auto enumerationJobInfos = m_testEnumerationJobInfoGenerator->GenerateJobInfos(testTargets);
            m_testEnumerator->Enumerate(
                enumerationJobInfos,
                StdOutputRouting::ToParent,
                StdErrorRouting::ToParent,
                AZStd::nullopt,
                AZStd::nullopt,
                AZStd::nullopt,
                stdPrint);
        }
        
        if (m_useNullTestRunner)
        {
            // We don't delete the artifacts as they have been left by another test runner (e.g. ctest)
            return GenerateInstrumentedRunResult(
            GenerateJobInfosAndRunTests(
                m_nullTestRunner.get(),
                m_testRunJobInfoGenerator.get(),
                testTargets,
                PythonInstrumentedTestRunnerErrorCodeChecker,
                executionFailurePolicy,
                testFailurePolicy,
                targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                callback,
                stdRoutingCallback),
            integrityFailurePolicy);
        }
        else
        {;
            DeleteArtifactXmls();
            return GenerateInstrumentedRunResult(
                GenerateJobInfosAndRunTests(
                    m_testRunner.get(),
                    m_testRunJobInfoGenerator.get(),
                    testTargets,
                    PythonInstrumentedTestRunnerErrorCodeChecker,
                    executionFailurePolicy,
                    testFailurePolicy,
                    targetOutputCapture,
                    testTargetTimeout,
                    globalTimeout,
                    callback,
                    stdRoutingCallback),
                integrityFailurePolicy);
        }
    }
} // namespace TestImpact
