/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/Python/TestImpactPythonRuntime.h>

#include <TestImpactRuntime.h>
#include <Artifact/Static/TestImpactPythonTestTargetMeta.h>
#include <Artifact/Factory/TestImpactPythonTestTargetMetaMapFactory.h>
#include <Dependency/TestImpactSourceCoveringTestsSerializer.h>
#include <Dependency/TestImpactTestSelectorAndPrioritizer.h>
#include <Target/Python/TestImpactPythonProductionTarget.h>
#include <Target/Python/TestImpactPythonTargetListCompiler.h>
#include <Target/Python/TestImpactPythonTestTarget.h>
#include <TestEngine/Python/TestImpactPythonTestEngine.h>

#include <AzCore/std/string/regex.h>

namespace TestImpact
{
    PythonTestTargetMetaMap ReadPythonTestTargetMetaMapFile(SuiteType suiteFilter, const RepoPath& testTargetMetaConfigFile, const AZStd::string& buildType)
    {
        const auto masterTestListData = ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        auto testTargetMetaMap = PythonTestTargetMetaMapFactory(masterTestListData, suiteFilter);
        for (auto& [name, meta] : testTargetMetaMap)
        {
            meta.m_scriptMeta.m_testCommand = AZStd::regex_replace(meta.m_scriptMeta.m_testCommand, AZStd::regex("\\$\\<CONFIG\\>"), buildType); 
        }

        return testTargetMetaMap;
    }

    PythonRuntime::PythonRuntime(
        PythonRuntimeConfig&& config,
        [[maybe_unused]] const AZStd::optional<RepoPath>& dataFile,
        [[maybe_unused]] const AZStd::optional<RepoPath>& previousRunDataFile,
        [[maybe_unused]] const AZStd::vector<ExcludedTarget>& testsToExclude,
        [[maybe_unused]] SuiteType suiteFilter,
        [[maybe_unused]] Policy::ExecutionFailure executionFailurePolicy,
        [[maybe_unused]] Policy::FailedTestCoverage failedTestCoveragePolicy,
        [[maybe_unused]] Policy::TestFailure testFailurePolicy,
        [[maybe_unused]] Policy::IntegrityFailure integrationFailurePolicy,
        [[maybe_unused]] Policy::TargetOutputCapture targetOutputCapture)
        : m_config(AZStd::move(config))
        , m_suiteFilter(suiteFilter)
        , m_executionFailurePolicy(executionFailurePolicy)
        , m_failedTestCoveragePolicy(failedTestCoveragePolicy)
        , m_testFailurePolicy(testFailurePolicy)
        , m_integrationFailurePolicy(integrationFailurePolicy)
        , m_targetOutputCapture(targetOutputCapture)
    {
        // Construct the build targets from the build target descriptors
        auto targetDescriptors = ReadTargetDescriptorFiles(m_config.m_commonConfig.m_buildTargetDescriptor);
        auto buildTargets = CompilePythonTargetLists(
            AZStd::move(targetDescriptors),
            ReadPythonTestTargetMetaMapFile(suiteFilter, m_config.m_commonConfig.m_testTargetMeta.m_metaFile, m_config.m_commonConfig.m_meta.m_buildConfig));
        auto&& [productionTargets, testTargets] = buildTargets;
        m_buildTargets = AZStd::make_unique<BuildTargetList<ProductionTarget, TestTarget>>(
            AZStd::move(testTargets), AZStd::move(productionTargets));

        // Construct the dynamic dependency map from the build targets
        m_dynamicDependencyMap = AZStd::make_unique<DynamicDependencyMap<ProductionTarget, TestTarget>>(m_buildTargets.get());

        // Construct the test selector and prioritizer from the dependency graph data (NOTE: currently not implemented)
        m_testSelectorAndPrioritizer = AZStd::make_unique<TestSelectorAndPrioritizer<ProductionTarget, TestTarget>>(*m_dynamicDependencyMap.get());

        // Construct the target exclude list from the exclude file if provided, otherwise use target configuration data
        if (!testsToExclude.empty())
        {
            // Construct using data from excludeTestFile
            m_testTargetExcludeList =
                ConstructTestTargetExcludeList(m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList(), testsToExclude);
        }
        else
        {
            // Construct using data from config file.
            m_testTargetExcludeList = ConstructTestTargetExcludeList(
                m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList(), m_config.m_target.m_excludedTargets);
        }

        // Construct the test engine with the workspace path and launcher binaries
        m_testEngine = AZStd::make_unique<PythonTestEngine>(
            m_config.m_commonConfig.m_repo.m_root,
            m_config.m_commonConfig.m_repo.m_build,
            m_config.m_workspace.m_temp,
            true);

        try
        {
            if (dataFile.has_value())
            {
                m_sparTiaFile = dataFile.value().String();
            }
            else
            {
                m_sparTiaFile = m_config.m_workspace.m_active.m_root / RepoPath(SuiteTypeAsString(m_suiteFilter)) /
                    m_config.m_workspace.m_active.m_sparTiaFile;
            }

            // Populate the dynamic dependency map with the existing source coverage data (if any)
            const auto tiaDataRaw = ReadFileContents<Exception>(m_sparTiaFile);
            const auto tiaData = DeserializeSourceCoveringTestsList(tiaDataRaw);
            if (tiaData.GetNumSources())
            {
                m_dynamicDependencyMap->ReplaceSourceCoverage(tiaData);
                m_hasImpactAnalysisData = true;

                // Enumerate new test targets
                // const auto testTargetsWithNoEnumeration = m_dynamicDependencyMap->GetNotCoveringTests();
                // if (!testTargetsWithNoEnumeration.empty())
                //{
                //    m_testEngine->UpdateEnumerationCache(
                //        testTargetsWithNoEnumeration,
                //        Policy::ExecutionFailure::Ignore,
                //        Policy::TestFailure::Continue,
                //        AZStd::nullopt,
                //        AZStd::nullopt,
                //        AZStd::nullopt);
                //}
            }
        }
        catch (const DependencyException& e)
        {
            if (integrationFailurePolicy == Policy::IntegrityFailure::Abort)
            {
                throw RuntimeException(e.what());
            }
        }
        catch ([[maybe_unused]] const Exception& e)
        {
            AZ_Printf(
                LogCallSite,
                AZStd::string::format(
                    "No test impact analysis data found for suite '%s' at %s\n",
                    SuiteTypeAsString(m_suiteFilter).c_str(),
                    m_sparTiaFile.c_str())
                    .c_str());
        }
    }

    PythonRuntime::~PythonRuntime() = default;

    AZStd::pair<AZStd::vector<const PythonTestTarget*>, AZStd::vector<const PythonTestTarget*>> PythonRuntime::SelectCoveringTestTargets(
        const ChangeList& changeList, Policy::TestPrioritization testPrioritizationPolicy)
    {
        AZStd::vector<const TestTarget*> discardedTestTargets;

        // Select and prioritize the test targets pertinent to this change list
        const auto changeDependencyList = m_dynamicDependencyMap->ApplyAndResoveChangeList(changeList, m_integrationFailurePolicy);
        const auto selectedTestTargets = m_testSelectorAndPrioritizer->SelectTestTargets(changeDependencyList, testPrioritizationPolicy);

        // Populate a set with the selected test targets so that we can infer the discarded test target not selected for this change list
        const AZStd::unordered_set<const TestTarget*> selectedTestTargetSet(selectedTestTargets.begin(), selectedTestTargets.end());

        // Update the enumeration caches of mutated targets regardless of the current sharding policy
        // EnumerateMutatedTestTargets(changeDependencyList);

        // The test targets in the main list not in the selected test target set are the test targets not selected for this change list
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
        {
            if (!selectedTestTargetSet.contains(&testTarget))
            {
                discardedTestTargets.push_back(&testTarget);
            }
        }

        return { selectedTestTargets, discardedTestTargets };
    }

    void PythonRuntime::ClearDynamicDependencyMapAndRemoveExistingFile()
    {
        m_dynamicDependencyMap->ClearAllSourceCoverage();
        DeleteFile(m_sparTiaFile);
    }

    PolicyStateBase PythonRuntime::GeneratePolicyStateBase() const
    {
        PolicyStateBase policyState;

        policyState.m_executionFailurePolicy = m_executionFailurePolicy;
        policyState.m_failedTestCoveragePolicy = m_failedTestCoveragePolicy;
        policyState.m_integrityFailurePolicy = m_integrationFailurePolicy;
        policyState.m_targetOutputCapture = m_targetOutputCapture;
        policyState.m_testFailurePolicy = m_testFailurePolicy;
        policyState.m_testShardingPolicy = Policy::TestSharding::Never;

        return policyState;
    }

    SequencePolicyState PythonRuntime::GenerateSequencePolicyState() const
    {
        return { GeneratePolicyStateBase() };
    }

    SafeImpactAnalysisSequencePolicyState PythonRuntime::GenerateSafeImpactAnalysisSequencePolicyState(
        Policy::TestPrioritization testPrioritizationPolicy) const
    {
        return { GeneratePolicyStateBase(), testPrioritizationPolicy };
    }

    ImpactAnalysisSequencePolicyState PythonRuntime::GenerateImpactAnalysisSequencePolicyState(
        Policy::TestPrioritization testPrioritizationPolicy, Policy::DynamicDependencyMap dynamicDependencyMapPolicy) const
    {
        return { GeneratePolicyStateBase(), testPrioritizationPolicy, dynamicDependencyMapPolicy };
    }

    Client::RegularSequenceReport PythonRuntime::RegularTestSequence(
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]] AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]] AZStd::optional<TestSequenceCompleteCallback<Client::RegularSequenceReport>> testSequenceEndCallback,
        [[maybe_unused]] AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        //const Timer sequenceTimer;
        //AZStd::vector<const NativeTestTarget*> includedTestTargets;
        //AZStd::vector<const NativeTestTarget*> excludedTestTargets;
        //
        //// Separate the test targets into those that are excluded by either the test filter or exclusion list and those that are not
        //for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargets()->GetTestTargetList().GetTargets())
        //{
        //    if (m_regularTestTargetExcludeList->IsTestTargetFullyExcluded(&testTarget))
        //    {
        //        excludedTestTargets.push_back(&testTarget);
        //    }
        //    else
        //    {
        //        includedTestTargets.push_back(&testTarget);
        //    }
        //}
        //
        //// Extract the client facing representation of selected test targets
        //const Client::TestRunSelection selectedTests(
        //    ExtractTestTargetNames(includedTestTargets), ExtractTestTargetNames(excludedTestTargets));
        //
        //// Inform the client that the sequence is about to start
        //if (testSequenceStartCallback.has_value())
        //{
        //    (*testSequenceStartCallback)(m_suiteFilter, selectedTests);
        //}
        //
        //// Run the test targets and collect the test run results
        //const Timer testRunTimer;
        //const auto [result, testJobs] = m_testEngine->RegularRun(
        //    includedTestTargets,
        //    m_executionFailurePolicy,
        //    m_testFailurePolicy,
        //    m_targetOutputCapture,
        //    testTargetTimeout,
        //    globalTimeout,
        //    TestRunCompleteCallbackHandler(includedTestTargets.size(), testCompleteCallback));
        //const auto testRunDuration = testRunTimer.GetElapsedMs();
        //
        //// Generate the sequence report for the client
        //const auto sequenceReport = Client::RegularSequenceReport(
        //    m_maxConcurrency,
        //    testTargetTimeout,
        //    globalTimeout,
        //    GenerateSequencePolicyState(),
        //    m_suiteFilter,
        //    selectedTests,
        //    GenerateTestRunReport(result, testRunTimer.GetStartTimePointRelative(sequenceTimer), testRunDuration, testJobs));
        //
        //// Inform the client that the sequence has ended
        //if (testSequenceEndCallback.has_value())
        //{
        //    (*testSequenceEndCallback)(sequenceReport);
        //}
        //
        //return sequenceReport;

        return Client::RegularSequenceReport(
            1,
            AZStd::nullopt,
            AZStd::nullopt,
            SequencePolicyState{},
            m_suiteFilter,
            Client::TestRunSelection(),
            Client::TestRunReport(
                TestSequenceResult::Success,
                AZStd::chrono::high_resolution_clock::time_point(),
                AZStd::chrono::milliseconds{ 0 },
                {},
                {},
                {},
                {},
                {}));
    }

    Client::ImpactAnalysisSequenceReport PythonRuntime::ImpactAnalysisTestSequence(
        [[maybe_unused]] const ChangeList& changeList,
        [[maybe_unused]] Policy::TestPrioritization testPrioritizationPolicy,
        [[maybe_unused]] Policy::DynamicDependencyMap dynamicDependencyMapPolicy,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]] AZStd::optional<ImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]] AZStd::optional<TestSequenceCompleteCallback<Client::ImpactAnalysisSequenceReport>> testSequenceEndCallback,
        [[maybe_unused]] AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;
        
        // Draft in the test targets that have no coverage entries in the dynamic dependency map
        //const AZStd::vector<const NativeTestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();
        
        //const auto selectCoveringTestTargetsAndPruneDraftedFromDiscarded =
        //    [this, &draftedTestTargets, &changeList, testPrioritizationPolicy]()
        //{
        //    // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        //    const auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargets(changeList, testPrioritizationPolicy);
        //
        //    const AZStd::unordered_set<const NativeTestTarget*> draftedTestTargetsSet(draftedTestTargets.begin(), draftedTestTargets.end());
        //
        //    AZStd::vector<const NativeTestTarget*> discardedNotDraftedTestTargets;
        //    for (const auto* testTarget : discardedTestTargets)
        //    {
        //        if (!draftedTestTargetsSet.count(testTarget))
        //        {
        //            discardedNotDraftedTestTargets.push_back(testTarget);
        //        }
        //    }
        //
        //    return AZStd::pair{ selectedTestTargets, discardedNotDraftedTestTargets };
        //};
        //
        //const auto [selectedTestTargets, discardedTestTargets] = selectCoveringTestTargetsAndPruneDraftedFromDiscarded();

        // draft previously failing tests????????

        // The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        const auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargets(changeList, testPrioritizationPolicy);
        
        // The subset of selected test targets that are not on the configuration's exclude list and those that are
        const auto [includedSelectedTestTargets, excludedSelectedTestTargets] =
            SelectTestTargetsByExcludeList(*m_testTargetExcludeList, selectedTestTargets);
        
        // Functor for running instrumented test targets
        const auto instrumentedTestRun = [this, &testTargetTimeout](
                                             const AZStd::vector<const TestTarget*>& testsTargets,
                                             TestRunCompleteCallbackHandler<TestTarget>& testRunCompleteHandler,
                                             AZStd::optional<AZStd::chrono::milliseconds> globalTimeout)
        {
            return m_testEngine->InstrumentedRun(
                testsTargets,
                m_executionFailurePolicy,
                m_integrationFailurePolicy,
                m_testFailurePolicy,
                m_targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                AZStd::ref(testRunCompleteHandler));
        };
        
        if (dynamicDependencyMapPolicy == Policy::DynamicDependencyMap::Update)
        {
            AZStd::optional<AZStd::function<void(const AZStd::vector<TestEngineInstrumentedRun<TestTarget, TestCoverage>>& jobs)>>
                updateCoverage = [this](const AZStd::vector<TestEngineInstrumentedRun<TestTarget, TestCoverage>>& jobs)
            {
                m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
                                              *m_dynamicDependencyMap.get(),
                                              jobs,
                                              m_failedTestCoveragePolicy,
                                              m_integrationFailurePolicy,
                                              m_config.m_commonConfig.m_repo.m_root,
                                              m_sparTiaFile)
                                              .value_or(m_hasImpactAnalysisData);
            };
        
            return ImpactAnalysisTestSequenceWrapper(
                1,
                GenerateImpactAnalysisSequencePolicyState(testPrioritizationPolicy, dynamicDependencyMapPolicy),
                m_suiteFilter,
                sequenceTimer,
                instrumentedTestRun,
                includedSelectedTestTargets,
                excludedSelectedTestTargets,
                discardedTestTargets,
                {}, // draftedTestTargets,
                testTargetTimeout,
                globalTimeout,
                testSequenceStartCallback,
                testSequenceEndCallback,
                testCompleteCallback,
                updateCoverage);
        }
        else
        {
            return ImpactAnalysisTestSequenceWrapper(
                1,
                GenerateImpactAnalysisSequencePolicyState(testPrioritizationPolicy, dynamicDependencyMapPolicy),
                m_suiteFilter,
                sequenceTimer,
                instrumentedTestRun,
                includedSelectedTestTargets,
                excludedSelectedTestTargets,
                discardedTestTargets,
                {}, // draftedTestTargets,
                testTargetTimeout,
                globalTimeout,
                testSequenceStartCallback,
                testSequenceEndCallback,
                testCompleteCallback,
                AZStd::optional<AZStd::function<void(const AZStd::vector<TestEngineInstrumentedRun<TestTarget, TestCoverage>>& jobs)>>{
                    AZStd::nullopt });
        }

        //return Client::ImpactAnalysisSequenceReport(
        //    1,
        //    AZStd::nullopt,
        //    AZStd::nullopt,
        //    ImpactAnalysisSequencePolicyState{},
        //    m_suiteFilter,
        //    Client::TestRunSelection(),
        //    {},
        //    {},
        //    Client::TestRunReport(
        //        TestSequenceResult::Success,
        //        AZStd::chrono::high_resolution_clock::time_point(),
        //        AZStd::chrono::milliseconds{ 0 },
        //        {},
        //        {},
        //        {},
        //        {},
        //        {}),
        //    Client::TestRunReport(
        //        TestSequenceResult::Success,
        //        AZStd::chrono::high_resolution_clock::time_point(),
        //        AZStd::chrono::milliseconds{ 0 },
        //        {},
        //        {},
        //        {},
        //        {},
        //        {}));
    }

    Client::SafeImpactAnalysisSequenceReport PythonRuntime::SafeImpactAnalysisTestSequence(
        [[maybe_unused]] const ChangeList& changeList,
        [[maybe_unused]] Policy::TestPrioritization testPrioritizationPolicy,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]] AZStd::optional<SafeImpactAnalysisTestSequenceStartCallback> testSequenceStartCallback,
        [[maybe_unused]] AZStd::optional<TestSequenceCompleteCallback<Client::SafeImpactAnalysisSequenceReport>> testSequenceEndCallback,
        [[maybe_unused]] AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        //const Timer sequenceTimer;
        //TestRunData<TestEngineInstrumentedRun<NativeTestTarget, TestCoverage>> selectedTestRunData, draftedTestRunData;
        //TestRunData<TestEngineRegularRun<NativeTestTarget>> discardedTestRunData;
        //AZStd::optional<AZStd::chrono::milliseconds> sequenceTimeout = globalTimeout;
        //
        //// Draft in the test targets that have no coverage entries in the dynamic dependency map
        //AZStd::vector<const NativeTestTarget*> draftedTestTargets = m_dynamicDependencyMap->GetNotCoveringTests();
        //
        //// The test targets that were selected for the change list by the dynamic dependency map and the test targets that were not
        //const auto [selectedTestTargets, discardedTestTargets] = SelectCoveringTestTargets(changeList, testPrioritizationPolicy);
        //
        //// The subset of selected test targets that are not on the configuration's exclude list and those that are
        //const auto [includedSelectedTestTargets, excludedSelectedTestTargets] =
        //    SelectTestTargetsByExcludeList(*m_instrumentedTestTargetExcludeList, selectedTestTargets);
        //
        //// The subset of discarded test targets that are not on the configuration's exclude list and those that are
        //const auto [includedDiscardedTestTargets, excludedDiscardedTestTargets] =
        //    SelectTestTargetsByExcludeList(*m_regularTestTargetExcludeList, discardedTestTargets);
        //
        //// Extract the client facing representation of selected, discarded and drafted test targets
        //const Client::TestRunSelection selectedTests(
        //    ExtractTestTargetNames(includedSelectedTestTargets), ExtractTestTargetNames(excludedSelectedTestTargets));
        //const Client::TestRunSelection discardedTests(
        //    ExtractTestTargetNames(includedDiscardedTestTargets), ExtractTestTargetNames(excludedDiscardedTestTargets));
        //const auto draftedTests = ExtractTestTargetNames(draftedTestTargets);
        //
        //// Inform the client that the sequence is about to start
        //if (testSequenceStartCallback.has_value())
        //{
        //    (*testSequenceStartCallback)(m_suiteFilter, selectedTests, discardedTests, draftedTests);
        //}
        //
        //// We share the test run complete handler between the selected, discarded and drafted test runs as to present them together as one
        //// continuous test sequence to the client rather than three discrete test runs
        //const size_t totalNumTestRuns =
        //    includedSelectedTestTargets.size() + draftedTestTargets.size() + includedDiscardedTestTargets.size();
        //TestRunCompleteCallbackHandler testRunCompleteHandler(totalNumTestRuns, testCompleteCallback);
        //
        //// Functor for running instrumented test targets
        //const auto instrumentedTestRun = [this, &testTargetTimeout, &sequenceTimeout, &testRunCompleteHandler](
        //                                     const AZStd::vector<const NativeTestTarget*>& testsTargets)
        //{
        //    return m_testEngine->InstrumentedRun(
        //        testsTargets,
        //        m_executionFailurePolicy,
        //        m_integrationFailurePolicy,
        //        m_testFailurePolicy,
        //        m_targetOutputCapture,
        //        testTargetTimeout,
        //        sequenceTimeout,
        //        AZStd::ref(testRunCompleteHandler));
        //};
        //
        //// Functor for running uninstrumented test targets
        //const auto regularTestRun = [this, &testTargetTimeout, &sequenceTimeout, &testRunCompleteHandler](
        //                                const AZStd::vector<const NativeTestTarget*>& testsTargets)
        //{
        //    return m_testEngine->RegularRun(
        //        testsTargets,
        //        m_executionFailurePolicy,
        //        m_testFailurePolicy,
        //        m_targetOutputCapture,
        //        testTargetTimeout,
        //        sequenceTimeout,
        //        AZStd::ref(testRunCompleteHandler));
        //};
        //
        //// Functor for running instrumented test targets
        //const auto gatherTestRunData =
        //    [&sequenceTimer](const AZStd::vector<const NativeTestTarget*>& testsTargets, const auto& testRunner, auto& testRunData)
        //{
        //    const Timer testRunTimer;
        //    testRunData.m_relativeStartTime = testRunTimer.GetStartTimePointRelative(sequenceTimer);
        //    auto [result, jobs] = testRunner(testsTargets);
        //    testRunData.m_result = result;
        //    testRunData.m_jobs = AZStd::move(jobs);
        //    testRunData.m_duration = testRunTimer.GetElapsedMs();
        //};
        //
        //if (!includedSelectedTestTargets.empty())
        //{
        //    // Run the selected test targets and collect the test run results
        //    gatherTestRunData(includedSelectedTestTargets, instrumentedTestRun, selectedTestRunData);
        //
        //    // Carry the remaining global sequence time over to the discarded test run
        //    if (globalTimeout.has_value())
        //    {
        //        const auto elapsed = selectedTestRunData.m_duration;
        //        sequenceTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
        //    }
        //}
        //
        //if (!includedDiscardedTestTargets.empty())
        //{
        //    // Run the discarded test targets and collect the test run results
        //    gatherTestRunData(includedDiscardedTestTargets, regularTestRun, discardedTestRunData);
        //
        //    // Carry the remaining global sequence time over to the drafted test run
        //    if (globalTimeout.has_value())
        //    {
        //        const auto elapsed = selectedTestRunData.m_duration + discardedTestRunData.m_duration;
        //        sequenceTimeout = elapsed < globalTimeout.value() ? globalTimeout.value() - elapsed : AZStd::chrono::milliseconds(0);
        //    }
        //}
        //
        //if (!draftedTestTargets.empty())
        //{
        //    // Run the drafted test targets and collect the test run results
        //    gatherTestRunData(draftedTestTargets, instrumentedTestRun, draftedTestRunData);
        //}
        //
        //// Generate the sequence report for the client
        //const auto sequenceReport = Client::SafeImpactAnalysisSequenceReport(
        //    m_maxConcurrency,
        //    testTargetTimeout,
        //    globalTimeout,
        //    GenerateSafeImpactAnalysisSequencePolicyState(testPrioritizationPolicy),
        //    m_suiteFilter,
        //    selectedTests,
        //    discardedTests,
        //    draftedTests,
        //    GenerateTestRunReport(
        //        selectedTestRunData.m_result,
        //        selectedTestRunData.m_relativeStartTime,
        //        selectedTestRunData.m_duration,
        //        selectedTestRunData.m_jobs),
        //    GenerateTestRunReport(
        //        discardedTestRunData.m_result,
        //        discardedTestRunData.m_relativeStartTime,
        //        discardedTestRunData.m_duration,
        //        discardedTestRunData.m_jobs),
        //    GenerateTestRunReport(
        //        draftedTestRunData.m_result,
        //        draftedTestRunData.m_relativeStartTime,
        //        draftedTestRunData.m_duration,
        //        draftedTestRunData.m_jobs));
        //
        //// Inform the client that the sequence has ended
        //if (testSequenceEndCallback.has_value())
        //{
        //    (*testSequenceEndCallback)(sequenceReport);
        //}
        //
        //m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
        //                              m_dynamicDependencyMap.get(),
        //                              ConcatenateVectors(selectedTestRunData.m_jobs, draftedTestRunData.m_jobs),
        //                              m_failedTestCoveragePolicy,
        //                              m_integrationFailurePolicy,
        //                              m_config.m_commonConfig.m_repo.m_root,
        //                              m_sparTiaFile)
        //                              .value_or(m_hasImpactAnalysisData);
        //
        //return sequenceReport;

        return Client::SafeImpactAnalysisSequenceReport(
            1,
            AZStd::nullopt,
            AZStd::nullopt,
            SafeImpactAnalysisSequencePolicyState{},
            m_suiteFilter,
            {},
            {},
            {},
            Client::TestRunReport(
                TestSequenceResult::Success,
                AZStd::chrono::high_resolution_clock::time_point(),
                AZStd::chrono::milliseconds{ 0 },
                {},
                {},
                {},
                {},
                {}),
            Client::TestRunReport(
                TestSequenceResult::Success,
                AZStd::chrono::high_resolution_clock::time_point(),
                AZStd::chrono::milliseconds{ 0 },
                {},
                {},
                {},
                {},
                {}),
            Client::TestRunReport(
                TestSequenceResult::Success,
                AZStd::chrono::high_resolution_clock::time_point(),
                AZStd::chrono::milliseconds{ 0 },
                {},
                {},
                {},
                {},
                {}));
    }

    Client::SeedSequenceReport PythonRuntime::SeededTestSequence(
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestSequenceStartCallback> testSequenceStartCallback,
        AZStd::optional<TestSequenceCompleteCallback<Client::SeedSequenceReport>> testSequenceEndCallback,
        AZStd::optional<TestRunCompleteCallback> testCompleteCallback)
    {
        const Timer sequenceTimer;
        AZStd::vector<const TestTarget*> includedTestTargets;
        AZStd::vector<const TestTarget*> excludedTestTargets;
        
        // Separate the test targets into those that are excluded by either the test filter or exclusion list and those that are not
        for (const auto& testTarget : m_dynamicDependencyMap->GetBuildTargetList()->GetTestTargetList().GetTargets())
        {
            if (m_testTargetExcludeList->IsTestTargetFullyExcluded(&testTarget))
            {
                excludedTestTargets.push_back(&testTarget);
            }
            else
            {
                includedTestTargets.push_back(&testTarget);
            }
        }
        
        // Extract the client facing representation of selected test targets
        Client::TestRunSelection selectedTests(ExtractTestTargetNames(includedTestTargets), ExtractTestTargetNames(excludedTestTargets));
        
        // Inform the client that the sequence is about to start
        if (testSequenceStartCallback.has_value())
        {
            (*testSequenceStartCallback)(m_suiteFilter, selectedTests);
        }
        
        // Run the test targets and collect the test run results
        const Timer testRunTimer;
        const auto [result, testJobs] = m_testEngine->InstrumentedRun(
            includedTestTargets,
            m_executionFailurePolicy,
            m_integrationFailurePolicy,
            m_testFailurePolicy,
            m_targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            TestRunCompleteCallbackHandler<TestTarget>(includedTestTargets.size(), testCompleteCallback));
        const auto testRunDuration = testRunTimer.GetElapsedMs();
        
        // Generate the sequence report for the client
        const auto sequenceReport = Client::SeedSequenceReport(
            1,
            testTargetTimeout,
            globalTimeout,
            GenerateSequencePolicyState(),
            m_suiteFilter,
            selectedTests,
            GenerateTestRunReport(result, testRunTimer.GetStartTimePointRelative(sequenceTimer), testRunDuration, testJobs));
        
        // Inform the client that the sequence has ended
        if (testSequenceEndCallback.has_value())
        {
            (*testSequenceEndCallback)(sequenceReport);
        }
                
        ClearDynamicDependencyMapAndRemoveExistingFile();

        ////////////////////////////////

        AZStd::unordered_map<BuildTarget<ProductionTarget, TestTarget>, AZStd::unordered_set<const TestTarget*>> coveredTargets;
        AZStd::unordered_map<BuildTarget<ProductionTarget, TestTarget>, AZStd::unordered_set<const TestTarget*>> discoveredTargets;

        const auto isCovered = [&](const BuildTarget<ProductionTarget, TestTarget>& buildTarget)
        {
            return coveredTargets.count(buildTarget) || discoveredTargets.count(buildTarget);
        };

        for (const auto& testJob : testJobs)
        {
            if (testJob.GetCoverge().has_value())
            {
                for (const auto& coveredModule : testJob.GetCoverge()->GetModuleCoverages())
                {
                    const auto moduleName = AZ::IO::Path(coveredModule.m_path.Stem()).String();
                    const auto buildTargetName =
                        m_dynamicDependencyMap->GetBuildTargetList()->GetTargetNameFromOutputNameOrThrow(moduleName);
                    auto buildTarget = m_dynamicDependencyMap->GetBuildTargetList()->GetBuildTargetOrThrow(buildTargetName);
                    coveredTargets[buildTarget].insert(testJob.GetTestTarget());
                }
            }
        }

        {
            const auto& buildGraph = m_buildTargets->GetBuildGraph();
            const auto coveredTarget = m_buildTargets->GetBuildTargetOrThrow("TestImpact.Frontend.Console.Python");
            buildGraph.WalkBuildDependencies(
                coveredTarget,
                [&](const BuildGraphVertex<ProductionTarget, TestTarget>& dependency, size_t level)
                {
                    AZ_Printf(
                        AZStd::string::format("--->%s", dependency.m_buildTarget.GetTarget()->GetName().c_str()).c_str(),
                        AZStd::string::format("Distance: %zu\n", level).c_str());
                    return BuildGraphVertexVisitResult::Continue;
                });
        }

        // For each covered target:
        // 1. Walk the dependencies
        // 2. If a given dependency is exclusively depended on by covered targets, add the dependency to the covered target list#
        // 3. Otherwise, stop walking the branch of that dependency
        const auto& buildGraph = m_buildTargets->GetBuildGraph();
        for (const auto& keyVal : coveredTargets)
        {
            // Structured bindings cannot be captured by lambdas
            const auto& coveredTarget = keyVal.first;
            const auto& coveringTestTargets = keyVal.second;

            //const auto coveredTarget = m_buildTargets->GetBuildTargetOrThrow("WhiteBox.Editor");
            AZ_Printf(coveredTarget.GetTarget()->GetName().c_str(), "Walking dependencies...\n");
            buildGraph.WalkBuildDependencies(
                coveredTarget,
                [&](const BuildGraphVertex<ProductionTarget, TestTarget>& dependency, [[maybe_unused]] size_t level)
                {
                    //AZ_Printf(
                    //    AZStd::string::format("--->%s", dependency.m_buildTarget.GetTarget()->GetName().c_str()).c_str(),
                    //    "Walking dependers...\n");
                    bool exclusive = true;
                    buildGraph.WalkBuildDependers(
                        dependency.m_buildTarget,
                        [&](const BuildGraphVertex<ProductionTarget, TestTarget>& depender, [[maybe_unused]] size_t level)
                        {
                            const auto editorTarget =
                                m_buildTargets->GetBuildTarget(depender.m_buildTarget.GetTarget()->GetName() + ".Editor");
                            if (editorTarget && isCovered(*editorTarget))
                            {
                                auto& discoveredCoveringTests = discoveredTargets[dependency.m_buildTarget];
                                for (const auto& coveringTestTarget : coveringTestTargets)
                                {
                                    discoveredCoveringTests.insert(coveringTestTarget);
                                }

                                return BuildGraphVertexVisitResult::Continue;
                            }

                            if (isCovered(depender.m_buildTarget))
                            {
                                return BuildGraphVertexVisitResult::Continue;
                            }

                            //AZ_Printf(
                            //    AZStd::string::format("**********>%s", depender.m_buildTarget.GetTarget()->GetName().c_str()).c_str(),
                            //    "Is not covered :(\n");
                            exclusive = false;
                            return BuildGraphVertexVisitResult::AbortGraphTraversal;
                        });

                    if (exclusive)
                    {
                        //AZ_Printf(
                        //    AZStd::string::format("--->%s", dependency.m_buildTarget.GetTarget()->GetName().c_str()).c_str(),
                        //    "Exclusively covered dependers found :)\n");
                        //discoveredTargets.insert(dependency.m_buildTarget);

                        auto& discoveredCoveringTests = discoveredTargets[dependency.m_buildTarget];
                        for (const auto& coveringTestTarget : coveringTestTargets)
                        {
                            discoveredCoveringTests.insert(coveringTestTarget);
                        }

                        return BuildGraphVertexVisitResult::Continue;
                    }

                    //AZ_Printf(
                    //    AZStd::string::format("--->%s", dependency.m_buildTarget.GetTarget()->GetName().c_str()).c_str(),
                    //    "Exclusively covered dependers not found :(\n");
                    return BuildGraphVertexVisitResult::AbortBranchTraversal;
                });
        }

        AZStd::unordered_map<const TestTarget*, AZStd::unordered_set<BuildTarget<ProductionTarget, TestTarget>>> expandedCoverage;

        AZ_Printf("", "Known covered targets:\n");
        for (const auto& [target, tests] : coveredTargets)
        {
            AZ_Printf("---->", "%s\n", target.GetTarget()->GetName().c_str());
            for (const auto& test : tests)
            {
                expandedCoverage[test].insert(target);
            }
        }

        AZ_Printf("", "Discovered targets:\n");
        for (const auto& [target, tests] : discoveredTargets)
        {
            AZ_Printf("---->", "%s\n", target.GetTarget()->GetName().c_str());
            for (const auto& test : tests)
            {
                expandedCoverage[test].insert(target);
            }
        }

        AZStd::vector<TestEngineInstrumentedRun<TestTarget, typename PythonTestEngine::TestCaseCoverageType>> expandedTestJobs;
        expandedTestJobs.reserve(testJobs.size());
        for (const auto& testJob : testJobs)
        {
            AZStd::optional<TestRun> run = testJob.GetTestRun();
            AZStd::vector<ModuleCoverage> moduleCoverages;
            const auto& thisCoveredTargets = expandedCoverage[testJob.GetTestTarget()];
            for (const auto& coveredTarget : thisCoveredTargets)
            {
                moduleCoverages.emplace_back(AZStd::string::format("%s.dll", coveredTarget.GetTarget()->GetOutputName().c_str()), AZStd::vector<SourceCoverage>{});
            }

            TestEngineInstrumentedRun<TestTarget, typename PythonTestEngine::TestCaseCoverageType> foo(
                TestEngineJob<TestTarget>(testJob),
                AZStd::pair<AZStd::optional<TestRun>, typename PythonTestEngine::TestCaseCoverageType>{
                    run, TestCoverage(AZStd::move(moduleCoverages)) });
            expandedTestJobs.push_back(foo);
        }
       
        ////////////////////////////////

        m_hasImpactAnalysisData = UpdateAndSerializeDynamicDependencyMap(
                                      *m_dynamicDependencyMap.get(),
                                      expandedTestJobs,
                                      //testJobs,
                                      m_failedTestCoveragePolicy,
                                      m_integrationFailurePolicy,
                                      m_config.m_commonConfig.m_repo.m_root,
                                      m_sparTiaFile)
                                      .value_or(m_hasImpactAnalysisData);
        
        return sequenceReport;
    }

    bool PythonRuntime::HasImpactAnalysisData() const
    {
        return m_hasImpactAnalysisData;
    }
} // namespace TestImpact
