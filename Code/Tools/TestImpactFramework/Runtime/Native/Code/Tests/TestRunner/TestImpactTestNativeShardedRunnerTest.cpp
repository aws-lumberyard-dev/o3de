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

#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumerationSerializer.h>
#include <TestRunner/Native/TestImpactNativeTestEnumerator.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h>
#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactUtils.h>
#include <Artifact/Factory/TestImpactNativeTestTargetMetaMapFactory.h>
#include <BuildTarget/Common/TestImpactBuildTarget.h>
#include <Dependency/TestImpactDependencyException.h>
#include <TestImpactRuntimeUtils.h>
#include <Target/Native/TestImpactNativeProductionTarget.h>
#include <Target/Native/TestImpactNativeTargetListCompiler.h>
#include <Target/Native/TestImpactNativeTestTarget.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    using TestEnumerator = TestImpact::NativeTestEnumerator;
    using InsstrumentedTestRunner = TestImpact::NativeInstrumentedTestRunner;
    using EnumerationTestJobInfoGenerator = TestImpact::NativeTestEnumerationJobInfoGenerator;
    using ShardedTestJobInfoGenerator = TestImpact::NativeShardedInstrumentedTestRunJobInfoGenerator;
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
            , m_config(
                  TestImpact::NativeRuntimeConfigurationFactory(
                      TestImpact::ReadFileContents<TestImpact::ConfigurationException>(LY_TEST_IMPACT_DEFAULT_CONFIG_FILE)))
        {
            m_enumerationTestJobInfoGenerator = AZStd::make_unique<EnumerationTestJobInfoGenerator>(
                m_config.m_target.m_outputDirectory, m_config.m_workspace.m_temp, m_config.m_testEngine.m_testRunner.m_binary);

            m_shardedTestJobInfoGenerator = AZStd::make_unique<ShardedTestJobInfoGenerator>(
                m_maxConcurrency,
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
        TestEnumerator m_testEnumerator;
        InsstrumentedTestRunner m_instrumentedTestRunner;
        AZStd::unique_ptr<EnumerationTestJobInfoGenerator> m_enumerationTestJobInfoGenerator;
        AZStd::unique_ptr<ShardedTestJobInfoGenerator> m_shardedTestJobInfoGenerator;
        AZStd::vector<AZStd::pair<TestImpact::RepoPath, TestImpact::RepoPath>> m_testTargetPaths;
        const size_t m_maxConcurrency = 8;
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

    TEST_F(TestEnumeratorFixture, FooBarBaz)
    {
        const auto testTarget = m_buildTargets->GetTestTargetList().GetTarget("AzCore.Tests");
        const auto enumJob = m_enumerationTestJobInfoGenerator->GenerateJobInfo(testTarget, { 1 });
        const auto enumResult = m_testEnumerator.Enumerate(
            { enumJob },
            TestImpact::StdOutputRouting::None,
            TestImpact::StdErrorRouting::None,
            AZStd::nullopt,
            AZStd::nullopt,
            AZStd::nullopt,
            AZStd::nullopt);

        const auto shardJob =
            m_shardedTestJobInfoGenerator->GenerateJobInfo(testTarget, enumResult.second.front().GetPayload().value(), { 10 });

        size_t completedJobs = 0;
        const auto jobCallback = [&]([[maybe_unused]] const typename InsstrumentedTestRunner::Job::Info& jobInfo,
                                  [[maybe_unused]] const TestImpact::JobMeta& meta,
                                  [[maybe_unused]] TestImpact::StdContent&& std)
        {
            AZ_Printf("Shard", "Shard %zu/%zu completed\n", ++completedJobs, m_maxConcurrency);
            if (std.m_out.has_value())
            {
                AZ_Printf("Out", "%s\n",  std.m_out->c_str());
            }
            if (std.m_err.has_value())
            {
                AZ_Printf("Out", "%s\n", std.m_err->c_str());
            }
            return TestImpact::ProcessCallbackResult::Continue;
        };

        const auto stdCallback = []([[maybe_unused]] const typename InsstrumentedTestRunner::Job::Info& jobInfo,
                                    [[maybe_unused]] const AZStd::string& stdOutput,
                                    [[maybe_unused]] const AZStd::string& stdError,
                                    [[maybe_unused]] AZStd::string&& stdOutDelta,
                                    [[maybe_unused]] AZStd::string&& stdErrDelta)
        {
            return TestImpact::ProcessCallbackResult::Continue;
        };

        const auto runResult = m_instrumentedTestRunner.RunTests(
            shardJob.second,
            TestImpact::StdOutputRouting::ToParent,
            TestImpact::StdErrorRouting::ToParent,
            AZStd::nullopt,
            AZStd::nullopt,
            jobCallback,
            stdCallback);

        EXPECT_TRUE(true);
    }
} // namespace UnitTest
