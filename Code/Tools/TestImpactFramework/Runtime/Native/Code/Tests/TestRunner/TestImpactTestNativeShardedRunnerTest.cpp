/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/Native/TestImpactNativeRuntimeConfigurationFactory.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <BuildTarget/Common/TestImpactBuildGraph.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumerationSerializer.h>
#include <TestRunner/Native/TestImpactNativeTestEnumerator.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h>
#include <TestRunner/Native/Shard/TestImpactNativeShardedTestSystem.h>
#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactUtils.h>
#include <Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <BuildTarget/Common/TestImpactBuildTargetList.h>
#include <Dependency/TestImpactDependencyException.h>
#include <TestImpactRuntimeUtils.h>
#include <Target/Native/TestImpactNativeProductionTarget.h>
#include <Target/Native/TestImpactNativeTargetListCompiler.h>
#include <Target/Native/TestImpactNativeTestTarget.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

#include <iostream>

namespace UnitTest
{
    namespace
    {
        bool operator==(const TestImpact::LineCoverage& lhs, const TestImpact::LineCoverage& rhs)
        {
            if (lhs.m_hitCount != rhs.m_hitCount)
            {
                AZ_Error("LineCoverage ==", false, "lhs.m_hitCount: %u, rhs.m_hitCount: %u", lhs.m_hitCount, rhs.m_hitCount);
                return false;
            }

            if (lhs.m_lineNumber != rhs.m_lineNumber)
            {
                AZ_Error("LineCoverage ==", false, "lhs.m_lineNumber: %u, rhs.m_lineNumber: %u", lhs.m_lineNumber, rhs.m_lineNumber);
                return false;
            }

            return true;
        }

        bool operator==(const TestImpact::SourceCoverage& lhs, const TestImpact::SourceCoverage& rhs)
        {
            if (lhs.m_path != rhs.m_path)
            {
                AZ_Error("LineCoverage ==", false, "lhs.m_path: %s, rhs.m_path: %s", lhs.m_path.c_str(), rhs.m_path.c_str());
                return false;
            }

            if (lhs.m_coverage.empty() != rhs.m_coverage.empty())
            {
                AZ_Error(
                    "LineCoverage ==",
                    false,
                    "lhs.m_coverage.empty(): %u, rhs.m_coverage.empty(): %u",
                    lhs.m_coverage.empty(),
                    rhs.m_coverage.empty());
                return false;
            }

            if (!lhs.m_coverage.empty())
            {
                return AZStd::equal(
                    lhs.m_coverage.begin(),
                    lhs.m_coverage.end(),
                    rhs.m_coverage.begin(),
                    [](const TestImpact::LineCoverage& left, const TestImpact::LineCoverage& right)
                    {
                        return left == right;
                    });
            }

            return true;
        }

        bool operator==(const TestImpact::ModuleCoverage& lhs, const TestImpact::ModuleCoverage& rhs)
        {
            if (lhs.m_path != rhs.m_path)
            {
                AZ_Error("ModuleCoverage ==", false, "lhs.m_path: %s, rhs.m_path: %s", lhs.m_path.c_str(), rhs.m_path.c_str());
                return false;
            }

            return AZStd::equal(
                lhs.m_sources.begin(),
                lhs.m_sources.end(),
                rhs.m_sources.begin(),
                [](const TestImpact::SourceCoverage& left, const TestImpact::SourceCoverage& right)
                {
                    return left == right;
                });
        }

        bool operator==(const AZStd::vector<TestImpact::ModuleCoverage>& lhs, const AZStd::vector<TestImpact::ModuleCoverage>& rhs)
        {
            if (lhs.size() != rhs.size())
            {
                AZ_Error("ModuleCoverage ==", false, "lhs.size(): %u, rhs.size(): %u", lhs.size(), rhs.size());
                return false;
            }

            return AZStd::equal(
                lhs.begin(),
                lhs.end(),
                rhs.begin(),
                [](const TestImpact::ModuleCoverage& left, const TestImpact::ModuleCoverage& right)
                {
                    return left == right;
                });
        }

        bool operator!=(const AZStd::vector<TestImpact::ModuleCoverage>& lhs, const AZStd::vector<TestImpact::ModuleCoverage>& rhs)
        {
            return !(lhs == rhs);
        }

        bool operator==(const TestImpact::TestCoverage& lhs, const TestImpact::TestCoverage& rhs)
        {
            if (lhs.GetNumModulesCovered() != rhs.GetNumModulesCovered())
            {
                return false;
            }

            if (lhs.GetNumSourcesCovered() != rhs.GetNumSourcesCovered())
            {
                return false;
            }

            if (lhs.GetModuleCoverages() != rhs.GetModuleCoverages())
            {
                return false;
            }

            if (lhs.GetSourcesCovered().size() != rhs.GetSourcesCovered().size())
            {
                return false;
            }

            return true;
        }
    }

    using TestEnumerator = TestImpact::NativeTestEnumerator;
    using InstrumentedTestRunner = TestImpact::NativeInstrumentedTestRunner;
    using EnumerationTestJobInfoGenerator = TestImpact::NativeTestEnumerationJobInfoGenerator;
    using InstrumentedShardedTestJobInfoGenerator = TestImpact::NativeShardedInstrumentedTestRunJobInfoGenerator;
    using InstrumentedTestJobInfoGenerator = TestImpact::NativeInstrumentedTestRunJobInfoGenerator;
    using InstrumentedShardedTestSystem = TestImpact::NativeShardedTestSystem<InstrumentedTestRunner>;
    using RepoPath =TestImpact::RepoPath;
    using RuntimeConfig = TestImpact::NativeRuntimeConfig;
    using TestTargetMetaMap = TestImpact::NativeTestTargetMetaMap;
    using SuiteSet = TestImpact::SuiteSet;
    using SuiteLabelExcludeSet = TestImpact::SuiteLabelExcludeSet;
    using TargetConfig = TestImpact::NativeTargetConfig;
    using RuntimeException = TestImpact::RuntimeException;
    using ProductionTarget = TestImpact::NativeProductionTarget;
    using TestTarget = TestImpact::NativeTestTarget;
    using BuildTargetList = TestImpact::BuildTargetList<ProductionTarget, TestTarget>;
    using DynamicDependencyMap = TestImpact::DynamicDependencyMap<ProductionTarget, TestTarget>;

    TestTargetMetaMap ReadNativeTestTargetMetaMapFile(
        const SuiteSet& suiteSet,
        const SuiteLabelExcludeSet& suiteLabelExcludeSet,
        const RepoPath& testTargetMetaConfigFile,
        const TargetConfig& targetConfig)
    {
        // todo: replace with hand-written map with the tiaf test targets
        const auto masterTestListData = TestImpact::ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        return NativeTestTargetMetaMapFactory(masterTestListData, suiteSet, suiteLabelExcludeSet, targetConfig);
    }

    class TestEnumeratorFixture
        : public LeakDetectionFixture
    {
    public:
        TestEnumeratorFixture()
            : m_testEnumerator(m_maxConcurrency)
            , m_instrumentedTestRunner(m_maxConcurrency)
            , m_instrumentedShardedTestSystem(m_instrumentedTestRunner)
            , m_config(
                  TestImpact::NativeRuntimeConfigurationFactory(
                      TestImpact::ReadFileContents<TestImpact::ConfigurationException>(LY_TEST_IMPACT_DEFAULT_CONFIG_FILE)))
        {
            m_enumerationTestJobInfoGenerator = AZStd::make_unique<EnumerationTestJobInfoGenerator>(
                m_config.m_target.m_outputDirectory, m_config.m_workspace.m_temp, m_config.m_testEngine.m_testRunner.m_binary);

            m_shardedTestJobInfoGenerator = AZStd::make_unique<InstrumentedShardedTestJobInfoGenerator>(
                m_maxConcurrency,
                m_config.m_commonConfig.m_repo.m_root,
                m_config.m_target.m_outputDirectory,
                m_config.m_shardedArtifactDir,
                m_config.m_testEngine.m_testRunner.m_binary,
                m_config.m_testEngine.m_instrumentation.m_binary);

            m_testJobInfoGenerator = AZStd::make_unique<InstrumentedTestJobInfoGenerator>(
                m_config.m_commonConfig.m_repo.m_root,
                m_config.m_target.m_outputDirectory,
                m_config.m_workspace.m_temp,
                m_config.m_testEngine.m_testRunner.m_binary,
                m_config.m_testEngine.m_instrumentation.m_binary);

            // Construct the build targets from the build target descriptors
            auto targetDescriptors = ReadTargetDescriptorFiles(m_config.m_commonConfig.m_buildTargetDescriptor);
            auto buildTargets = TestImpact::CompileNativeTargetLists(
                AZStd::move(targetDescriptors),
                ReadNativeTestTargetMetaMapFile({ "main" }, {}, m_config.m_commonConfig.m_testTargetMeta.m_metaFile, m_config.m_target));
            auto&& [productionTargets, testTargets] = buildTargets;
            m_buildTargets =
                AZStd::make_unique<BuildTargetList>(AZStd::move(testTargets), AZStd::move(productionTargets));

            // Construct the dynamic dependency map from the build targets
            m_dynamicDependencyMap = AZStd::make_unique<DynamicDependencyMap>(m_buildTargets.get());
        }

        void SetUp() override;

    protected:
        static constexpr size_t m_maxConcurrency = 32;
        TestEnumerator m_testEnumerator;
        InstrumentedTestRunner m_instrumentedTestRunner;
        InstrumentedShardedTestSystem m_instrumentedShardedTestSystem;
        AZStd::unique_ptr<EnumerationTestJobInfoGenerator> m_enumerationTestJobInfoGenerator;
        AZStd::unique_ptr<InstrumentedShardedTestJobInfoGenerator> m_shardedTestJobInfoGenerator;
        AZStd::unique_ptr<InstrumentedTestJobInfoGenerator> m_testJobInfoGenerator;
        AZStd::vector<AZStd::pair<TestImpact::RepoPath, TestImpact::RepoPath>> m_testTargetPaths;
        RuntimeConfig m_config;
        AZStd::unique_ptr<DynamicDependencyMap> m_dynamicDependencyMap;
        AZStd::unique_ptr<BuildTargetList> m_buildTargets;
    };

    void TestEnumeratorFixture::SetUp()
    {
        TestImpact::DeleteFiles(LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR, "*.*");

        // first: path to test target bin
        // second: path to test target gtest enumeration file in XML format
        const AZStd::string enumPath = AZStd::string(LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR) + "/%s.Enumeration.xml";
        m_testTargetPaths.emplace_back(
            LY_TEST_IMPACT_TEST_TARGET_A_BIN, AZStd::string::format(enumPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_A_BASE_NAME));
        m_testTargetPaths.emplace_back(
            LY_TEST_IMPACT_TEST_TARGET_B_BIN, AZStd::string::format(enumPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_B_BASE_NAME));
        m_testTargetPaths.emplace_back(
            LY_TEST_IMPACT_TEST_TARGET_C_BIN, AZStd::string::format(enumPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_C_BASE_NAME));
        m_testTargetPaths.emplace_back(
            LY_TEST_IMPACT_TEST_TARGET_D_BIN, AZStd::string::format(enumPath.c_str(), LY_TEST_IMPACT_TEST_TARGET_D_BASE_NAME));
    }

    template<typename TestRunnerType>
    class TestRunnerHandler
        : private TestImpact::NativeShardedTestSystemNotificationsBus<TestRunnerType>::Handler
        , private TestRunnerType::NotificationsBus::Handler
    {
    public:
        TestRunnerHandler()
        {
            TestImpact::NativeShardedTestSystemNotificationsBus<TestRunnerType>::Handler::BusConnect();
            TestRunnerType::NotificationsBus::Handler::BusConnect();
        }

        ~TestRunnerHandler()
        {
            TestRunnerType::NotificationsBus::Handler::BusDisconnect();
            TestImpact::NativeShardedTestSystemNotificationsBus<TestRunnerType>::Handler::BusDisconnect();
        }
    private:
        TestImpact::ProcessCallbackResult OnShardedJobComplete(
            [[maybe_unused]] const typename TestRunnerType::Job::Info& jobInfo,
            [[maybe_unused]] const TestImpact::JobMeta& meta,
            [[maybe_unused]] const TestImpact::StdContent& std) override
        {
            //AZ_Printf("Test Target", "Complete!\n");
            //if (std.m_err.has_value())
            //{
            //    std::cout << std.m_err->c_str() << "\n";
            //}
            //if (std.m_out.has_value())
            //{
            //    std::cout << std.m_out->c_str() << "\n";
            //}
            return TestImpact::ProcessCallbackResult::Continue;
        }

        TestImpact::ProcessCallbackResult OnShardedSubJobComplete(
            [[maybe_unused]] typename TestRunnerType::JobInfo::Id jobId,
            [[maybe_unused]] size_t subJobCount,
            [[maybe_unused]] const typename TestRunnerType::Job::Info& subJobInfo,
            [[maybe_unused]] const TestImpact::JobMeta& subJobMeta,
            [[maybe_unused]] const TestImpact::StdContent& subJobStd) override
        {
            return TestImpact::ProcessCallbackResult::Continue;
        }

        TestImpact::ProcessCallbackResult OnJobComplete(
            [[maybe_unused]] const typename TestRunnerType::Job::Info& jobInfo,
            [[maybe_unused]] const TestImpact::JobMeta& meta,
            [[maybe_unused]] const TestImpact::StdContent& std) override
        {
            return TestImpact::ProcessCallbackResult::Continue;
        }
    };

    TEST_F(TestEnumeratorFixture, FooBarBaz)
    {
        const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("TestImpact.TestTargetA.Tests");
        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("TestImpact.TestTargetD.Tests");
        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("AzCore.Tests");
        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("AzToolsFramework.Tests");
        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("AzTestRunner.Tests");
        const auto enumJob = m_enumerationTestJobInfoGenerator->GenerateJobInfo(testTarget, { 1 });
        const auto enumResult = m_testEnumerator.Enumerate(
            { enumJob },
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::None,
            AZStd::nullopt,
            AZStd::nullopt);

        const auto job =
            m_testJobInfoGenerator->GenerateJobInfo(testTarget, { 10 });

        const auto shardJob =
            m_shardedTestJobInfoGenerator->GenerateJobInfo(testTarget, enumResult.second.front().GetPayload().value(), { 10 });

        TestImpact::Timer timer;
        const auto runResult1 = m_instrumentedTestRunner.RunTests(
            { job },
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::ToParent,
            AZStd::nullopt,
            AZStd::nullopt);

        std::cout << "Duration 1: " << timer.GetElapsedMs().count() << "\n";
        timer.ResetStartTimePoint();

        TestRunnerHandler<InstrumentedTestRunner> handler;
        const auto runResult2 = m_instrumentedShardedTestSystem.RunTests(
            { shardJob },
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::ToParent,
            AZStd::nullopt,
            AZStd::nullopt);

        std::cout << "Duration 2: " << timer.GetElapsedMs().count() << "\n";
        
        EXPECT_TRUE(runResult1.second.front().GetPayload()->second.GetModuleCoverages() == runResult2.second.front().GetPayload()->second.GetModuleCoverages());
    }
} // namespace UnitTest
