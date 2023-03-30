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
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/TestImpactNativeShardedInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeShardedRegularTestRunner.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h>
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
#include <TestRunner/Common/Run/TestImpactTestCoverageSerializer.h>
#include <TestRunner/Common/Run/TestImpactTestRunSerializer.h>

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

            if (lhs.m_lineCoverage.empty() != rhs.m_lineCoverage.empty())
            {
                AZ_Error(
                    "LineCoverage ==",
                    false,
                    "lhs.m_coverage.empty(): %u, rhs.m_coverage.empty(): %u",
                    lhs.m_lineCoverage.empty(),
                    rhs.m_lineCoverage.empty());
                return false;
            }

            if (!lhs.m_lineCoverage.empty())
            {
                return AZStd::equal(
                    lhs.m_lineCoverage.begin(),
                    lhs.m_lineCoverage.end(),
                    rhs.m_lineCoverage.begin(),
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
    using RegularTestRunner = TestImpact::NativeRegularTestRunner;
    using EnumerationTestJobInfoGenerator = TestImpact::NativeTestEnumerationJobInfoGenerator;
    using ShardedInstrumentedTestJobInfoGenerator = TestImpact::NativeShardedInstrumentedTestRunJobInfoGenerator;
    using ShardedRegularTestJobInfoGenerator = TestImpact::NativeShardedRegularTestRunJobInfoGenerator;
    using InstrumentedTestJobInfoGenerator = TestImpact::NativeInstrumentedTestRunJobInfoGenerator;
    using RegularTestJobInfoGenerator = TestImpact::NativeRegularTestRunJobInfoGenerator;
    using ShardedInstrumentedTestRunner = TestImpact::NativeShardedInstrumentedTestRunner;
    using ShardedRegularTestRunner = TestImpact::NativeShardedRegularTestRunner;
    using RepoPath = TestImpact::RepoPath;
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
        const RepoPath& testTargetMetaConfigFile)
    {
        // todo: replace with hand-written map with the tiaf test targets
        const auto masterTestListData = TestImpact::ReadFileContents<RuntimeException>(testTargetMetaConfigFile);
        return TestImpact::NativeTestTargetMetaMapFactory(masterTestListData, suiteSet, suiteLabelExcludeSet);
    }

    class TestEnumeratorFixture
        : public LeakDetectionFixture
    {
    public:
        TestEnumeratorFixture()
            : m_testEnumerator(m_maxConcurrency)
            , m_instrumentedTestRunner(m_maxConcurrency)
            , m_regularTestRunner(m_maxConcurrency)
            , m_config(
                  TestImpact::NativeRuntimeConfigurationFactory(
                      TestImpact::ReadFileContents<TestImpact::ConfigurationException>(LY_TEST_IMPACT_DEFAULT_CONFIG_FILE)))
        {
            DeleteFiles(m_config.m_shardedArtifactDir.m_shardedTestRunArtifactDirectory, "*.*");
            DeleteFiles(m_config.m_shardedArtifactDir.m_shardedCoverageArtifactDirectory, "*.*");
            DeleteFiles(m_config.m_workspace.m_temp.m_testRunArtifactDirectory, "*.xml");

            m_instrumentedShardedTestRunner = AZStd::make_unique<ShardedInstrumentedTestRunner>(
                m_instrumentedTestRunner, m_config.m_commonConfig.m_repo.m_root, m_config.m_workspace.m_temp);

            m_regularShardedTestRunner = AZStd::make_unique<ShardedRegularTestRunner>(
                m_regularTestRunner, m_config.m_commonConfig.m_repo.m_root, m_config.m_workspace.m_temp);

            m_enumerationTestJobInfoGenerator = AZStd::make_unique<EnumerationTestJobInfoGenerator>(
                m_config.m_target.m_outputDirectory, m_config.m_workspace.m_temp, m_config.m_testEngine.m_testRunner.m_binary);

            m_instrumentedTestJobInfoGenerator = AZStd::make_unique<InstrumentedTestJobInfoGenerator>(
                m_config.m_commonConfig.m_repo.m_root,
                m_config.m_target.m_outputDirectory,
                m_config.m_workspace.m_temp,
                m_config.m_testEngine.m_testRunner.m_binary,
                m_config.m_testEngine.m_instrumentation.m_binary);

            m_regularTestJobInfoGenerator = AZStd::make_unique<RegularTestJobInfoGenerator>(
                m_config.m_commonConfig.m_repo.m_root,
                m_config.m_target.m_outputDirectory,
                m_config.m_workspace.m_temp,
                m_config.m_testEngine.m_testRunner.m_binary);

            m_shardedInstrumentedTestJobInfoGenerator = AZStd::make_unique<ShardedInstrumentedTestJobInfoGenerator>(
                *m_instrumentedTestJobInfoGenerator.get(),
                m_maxConcurrency,
                m_config.m_commonConfig.m_repo.m_root,
                m_config.m_target.m_outputDirectory,
                m_config.m_shardedArtifactDir,
                m_config.m_testEngine.m_testRunner.m_binary,
                m_config.m_testEngine.m_instrumentation.m_binary);

            m_shardedRegularTestJobInfoGenerator = AZStd::make_unique<ShardedRegularTestJobInfoGenerator>(
                *m_regularTestJobInfoGenerator.get(),
                m_maxConcurrency,
                m_config.m_commonConfig.m_repo.m_root,
                m_config.m_target.m_outputDirectory,
                m_config.m_shardedArtifactDir,
                m_config.m_testEngine.m_testRunner.m_binary);

            // Construct the build targets from the build target descriptors
            auto targetDescriptors = ReadTargetDescriptorFiles(m_config.m_commonConfig.m_buildTargetDescriptor);
            auto buildTargets = TestImpact::CompileNativeTargetLists(
                AZStd::move(targetDescriptors),
                ReadNativeTestTargetMetaMapFile({ "main" }, {}, m_config.m_commonConfig.m_testTargetMeta.m_metaFile));
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
        RegularTestRunner m_regularTestRunner;
        AZStd::unique_ptr<EnumerationTestJobInfoGenerator> m_enumerationTestJobInfoGenerator;
        AZStd::unique_ptr<ShardedInstrumentedTestRunner> m_instrumentedShardedTestRunner;
        AZStd::unique_ptr<ShardedRegularTestRunner> m_regularShardedTestRunner;
        AZStd::unique_ptr<ShardedInstrumentedTestJobInfoGenerator> m_shardedInstrumentedTestJobInfoGenerator;
        AZStd::unique_ptr<ShardedRegularTestJobInfoGenerator> m_shardedRegularTestJobInfoGenerator;
        AZStd::unique_ptr<InstrumentedTestJobInfoGenerator> m_instrumentedTestJobInfoGenerator;
        AZStd::unique_ptr<RegularTestJobInfoGenerator> m_regularTestJobInfoGenerator;
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

    TestImpact::StdContent stdContent;

    template<typename ShardedTestRunnerType>
    class TestRunnerHandler
        : private ShardedTestRunnerType::NotificationBus::Handler
    {
    public:
        TestRunnerHandler()
        {
            ShardedTestRunnerType::NotificationBus::Handler::BusConnect();
        }

        ~TestRunnerHandler()
        {
            ShardedTestRunnerType::NotificationBus::Handler::BusDisconnect();
        }
    private:
        TestImpact::ProcessCallbackResult OnJobComplete(
            [[maybe_unused]] const typename ShardedTestRunnerType::JobInfo& jobInfo,
            [[maybe_unused]] const TestImpact::JobMeta& meta,
            [[maybe_unused]] const TestImpact::StdContent& std) override
        {
            AZ_Printf("Test Target", "Complete!\n");
            //if (std.m_err.has_value())
            //{
            //    std::cout << std.m_err->c_str() << "\n";
            //}
            //if (std.m_out.has_value())
            //{
            //    std::cout << std.m_out->c_str() << "\n";
            //}

            stdContent = std;
            return TestImpact::ProcessCallbackResult::Continue;
        }

        TestImpact::ProcessCallbackResult OnShardedJobComplete(
            [[maybe_unused]] typename ShardedTestRunnerType::JobInfo::Id jobId,
            [[maybe_unused]] size_t subJobCount,
            [[maybe_unused]] const typename ShardedTestRunnerType::JobInfo& subJobInfo,
            [[maybe_unused]] const TestImpact::JobMeta& subJobMeta,
            [[maybe_unused]] const TestImpact::StdContent& subJobStd) override
        {
            return TestImpact::ProcessCallbackResult::Continue;
        }
    };

    TEST_F(TestEnumeratorFixture, FooBarBaz)
    {
        const auto path = m_config.m_commonConfig.m_repo.m_root / m_config.m_workspace.m_active.m_sparTiaFile;
        const auto rootName = path.RelativePath().String();

        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("AzTestRunner.Tests");
        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("TestImpact.TestTargetA.Tests");
        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("TestImpact.TestTargetD.Tests");
        const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("AzCore.Tests");
        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("AzToolsFramework.Tests");
        //const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("AzTestRunner.Tests");
        const auto enumJob = m_enumerationTestJobInfoGenerator->GenerateJobInfos({ testTarget });
        const auto enumResult = m_testEnumerator.Enumerate(
            { enumJob },
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::None,
            AZStd::nullopt,
            AZStd::nullopt);

        {
            const auto instrumentedJob = m_instrumentedTestJobInfoGenerator->GenerateJobInfo(testTarget, { 10 });
        
            const auto shardedInstrumentedJobs = m_shardedInstrumentedTestJobInfoGenerator->GenerateJobInfos(
                { TestImpact::TestTargetAndEnumeration{ testTarget, enumResult.second.front().GetPayload() } });
        
            TestImpact::Timer timer;
            //const auto runResult1 = m_instrumentedTestRunner.RunTests(
            //    { instrumentedJob },
            //    TestImpact::StdOutputRouting::ToParent,
            //    TestImpact::StdErrorRouting::ToParent,
            //    AZStd::nullopt,
            //    AZStd::nullopt);
            //
            ////{
            ////    TestImpact::WriteFileContents<TestImpact::TestRunnerException>(
            ////        TestImpact::Cobertura::SerializeTestCoverage(
            ////            runResult1.second.front().GetPayload()->second, m_config.m_commonConfig.m_repo.m_root),
            ////        m_config.m_workspace.m_temp.m_coverageArtifactDirectory / RepoPath(testTarget->GetName() + ".r1.xml"));
            ////}
            //
            //std::cout << "Duration 1: " << timer.GetElapsedMs().count() << "\n";
            //
            //timer.ResetStartTimePoint();
            
            TestRunnerHandler<ShardedInstrumentedTestRunner> handler;
            const auto runResult2 = m_instrumentedShardedTestRunner->RunTests(
                shardedInstrumentedJobs,
                TestImpact::StdOutputRouting::ToParent,
                TestImpact::StdErrorRouting::ToParent,
                AZStd::nullopt,
                AZStd::nullopt);
            
            //{
            //    TestImpact::WriteFileContents<TestImpact::TestRunnerException>(
            //        TestImpact::Cobertura::SerializeTestCoverage(
            //            runResult2.second.front().GetPayload()->second, m_config.m_commonConfig.m_repo.m_root),
            //        m_config.m_workspace.m_temp.m_coverageArtifactDirectory / RepoPath(testTarget->GetName() + ".r2.xml"));
            //}
            
            std::cout << "Duration 2: " << timer.GetElapsedMs().count() << "\n";
            
            const auto testString = TestImpact::GTest::SerializeTestRun(runResult2.second.front().GetPayload()->first.value());
            const auto coverageString = TestImpact::Cobertura::SerializeTestCoverage(
                runResult2.second.front().GetPayload()->second, m_config.m_commonConfig.m_repo.m_root);
            
            //EXPECT_TRUE(
            //    runResult1.second.front().GetPayload()->second.GetModuleCoverages() ==
            //    runResult2.second.front().GetPayload()->second.GetModuleCoverages());
        }

        {
            const auto regularJob = m_regularTestJobInfoGenerator->GenerateJobInfo(testTarget, { 10 });
        
            const auto shardedRegularJobs = m_shardedRegularTestJobInfoGenerator->GenerateJobInfos(
                { TestImpact::TestTargetAndEnumeration{ testTarget, enumResult.second.front().GetPayload() } });
        
            TestImpact::Timer timer;
            //const auto runResult1 = m_regularTestRunner.RunTests(
            //    { regularJob },
            //    TestImpact::StdOutputRouting::ToParent,
            //    TestImpact::StdErrorRouting::ToParent,
            //    AZStd::nullopt,
            //    AZStd::nullopt);
            //
            //std::cout << "Duration 1: " << timer.GetElapsedMs().count() << "\n";
            //
            //timer.ResetStartTimePoint();
        
            TestRunnerHandler<ShardedRegularTestRunner> handler;
            const auto runResult2 = m_regularShardedTestRunner->RunTests(
                shardedRegularJobs,
                TestImpact::StdOutputRouting::ToParent,
                TestImpact::StdErrorRouting::ToParent,
                AZStd::nullopt,
                AZStd::nullopt);
        
            std::cout << "Duration 2: " << timer.GetElapsedMs().count() << "\n";

            if (stdContent.m_err.has_value())
            {
                std::cout << stdContent.m_err->c_str() << "\n";
            }
            if (stdContent.m_out.has_value())
            {
                std::cout << stdContent.m_out->c_str() << "\n";
            }

            const auto testString = TestImpact::GTest::SerializeTestRun(runResult2.second.front().GetPayload().value());
        }

        return;
    }
} // namespace UnitTest
