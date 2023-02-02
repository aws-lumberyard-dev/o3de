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

#include <TestRunner/Common/Enumeration/TestImpactTestEnumerationSerializer.h>
#include <TestRunner/Native/TestImpactNativeTestEnumerator.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestImpactFramework/TestImpactConfiguration.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    using TestEnumerator = TestImpact::NativeTestEnumerator;
    using EnumerationTestJobInfoGenerator = typename TestImpact::NativeTestEnumerationJobInfoGenerator;
    using RepoPath =TestImpact::RepoPath;
    using RuntimeConfig = TestImpact::NativeRuntimeConfig;

    class TestEnumeratorFixture
        : public LeakDetectionFixture
    {
    public:
        TestEnumeratorFixture()
            : m_testEnumerator(m_maxConcurrency)
            , m_artifactDir{ RepoPath(LY_TEST_IMPACT_TEST_TARGET_RESULTS_DIR),
                             RepoPath(LY_TEST_IMPACT_TEST_TARGET_COVERAGE_DIR),
                             RepoPath(LY_TEST_IMPACT_TEST_TARGET_ENUMERATION_DIR) }
            , m_enumerationTestJobInfoGenerator(
                  RepoPath(LY_TEST_IMPACT_TEST_BIN_DIR), m_artifactDir, RepoPath(LY_TEST_IMPACT_AZ_TESTRUNNER_BIN))
            , m_config(
                  TestImpact::NativeRuntimeConfigurationFactory(
                      TestImpact::ReadFileContents<TestImpact::ConfigurationException>(LY_TEST_IMPACT_DEFAULT_CONFIG_FILE)))
        {
        }

        void SetUp() override;

    protected:
        TestImpact::ArtifactDir m_artifactDir;
        TestEnumerator m_testEnumerator;
        EnumerationTestJobInfoGenerator m_enumerationTestJobInfoGenerator;
        AZStd::vector<AZStd::pair<TestImpact::RepoPath, TestImpact::RepoPath>> m_testTargetPaths;
        const size_t m_maxConcurrency = 1;
        RuntimeConfig m_config;
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

    }
} // namespace UnitTest
