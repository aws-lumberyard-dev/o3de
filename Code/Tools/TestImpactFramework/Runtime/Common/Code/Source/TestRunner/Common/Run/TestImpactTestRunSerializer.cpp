/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/TestImpactTestRunnerException.h>
#include <TestRunner/Common/Run/TestImpactTestRunSerializer.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Date/DateFormat.h>
#include <AzCore/std/chrono/chrono.h>

namespace TestImpact
{
    namespace TestRunFields
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "suites",
            "name",
            "enabled",
            "tests",
            "duration",
            "status",
            "result"
        };

        enum
        {
            SuitesKey,
            NameKey,
            EnabledKey,
            TestsKey,
            DurationKey,
            StatusKey,
            ResultKey
        };
    } // namespace

    namespace GTest
    {
        // Keys for pertinent JSON node and attribute names
        constexpr const char* Keys[] =
        {
            "testsuites",
            "testsuite",
            "testcase",
            "failures",
            "failure",
            "disabled",
            "errors",
            "timestamp",
            "time",
            "classname",
            "message",
            "type"
        };
        
        enum
        {
            TestSuitesKey,
            TestSuiteKey,
            TestCaseKey,
            FailuresKey,
            FailureKey,
            DisabledKey,
            ErrorsKey,
            TimestampKey,
            TimeKey,
            ClassNameKey,
            MessageKey,
            FailureTypeKey
        };
        
        AZStd::string SerializeTestRun(const TestRun& testRun)
        {
            AZ::rapidxml::xml_document<> doc;
        
            AZ::rapidxml::xml_node<>* decl = doc.allocate_node(AZ::rapidxml::node_declaration);
            decl->append_attribute(doc.allocate_attribute("version", "1.0"));
            decl->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
            doc.append_node(decl);

            AZ::Date::Iso8601TimestampString iso8601Timestamp;
            AZ::Date::GetIso8601ExtendedFormatNow(iso8601Timestamp);
        
            AZ::rapidxml::xml_node<>* rootNode = doc.allocate_node(AZ::rapidxml::node_element, GTest::Keys[GTest::TestSuitesKey]);

            // Number of tests in all test suites
            rootNode->append_attribute(doc.allocate_attribute(
                TestRunFields::Keys[TestRunFields::TestsKey], doc.allocate_string(AZStd::to_string(testRun.GetNumTests()).c_str())));

            // Number of failures in all test suites
            rootNode->append_attribute(doc.allocate_attribute(
                GTest::Keys[GTest::FailuresKey], doc.allocate_string(AZStd::to_string(testRun.GetNumFailures()).c_str())));

            // Number of disabled tests in all test suites
            rootNode->append_attribute(doc.allocate_attribute(
                GTest::Keys[GTest::DisabledKey], doc.allocate_string(AZStd::to_string(testRun.GetNumDisabledTests()).c_str())));

            // Number of errors in all test suites (not supported in TestRun)
            rootNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::ErrorsKey], "0"));

            // Timestamp of test completion (aka time of serialization)
            rootNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::TimestampKey], iso8601Timestamp.c_str()));

            // Total duration of all test suites
            rootNode->append_attribute(doc.allocate_attribute(
                GTest::Keys[GTest::TimeKey], doc.allocate_string(AZStd::to_string(testRun.GetDuration().count() / 1000.f).c_str())));

            // Name (unclear, all seem to be AllTests)
            rootNode->append_attribute(doc.allocate_attribute(TestRunFields::Keys[TestRunFields::NameKey], "AllTests"));
            doc.append_node(rootNode);

            // Individual test suites
            for (const auto& testSuite : testRun.GetTestSuites())
            {
                AZ::rapidxml::xml_node<>* testSuiteNode = doc.allocate_node(AZ::rapidxml::node_element, GTest::Keys[GTest::TestSuiteKey]);

                // Name of test suite
                testSuiteNode->append_attribute(
                    doc.allocate_attribute(TestRunFields::Keys[TestRunFields::NameKey], testSuite.m_name.c_str()));

                // Number of tests in test suite
                testSuiteNode->append_attribute(doc.allocate_attribute(
                    TestRunFields::Keys[TestRunFields::TestsKey], doc.allocate_string(AZStd::to_string(testSuite.m_tests.size()).c_str())));

                // Number of failures in test suite
                const auto numFailingTests = AZStd::count_if(
                    testSuite.m_tests.begin(),
                    testSuite.m_tests.end(),
                    [](const TestRunCase& test)
                    {
                        return test.m_result.has_value() && test.m_result.value() == TestRunResult::Failed;
                    });
                testSuiteNode->append_attribute(doc.allocate_attribute(
                    GTest::Keys[GTest::FailuresKey], doc.allocate_string(AZStd::to_string(numFailingTests).c_str())));

                // Number of disabled tests in test suite
                const auto numDisabledTests = AZStd::count_if(
                    testSuite.m_tests.begin(),
                    testSuite.m_tests.end(),
                    [](const TestRunCase& test)
                    {
                        return !test.m_enabled;
                    });
                testSuiteNode->append_attribute(doc.allocate_attribute(
                    GTest::Keys[GTest::DisabledKey], doc.allocate_string(AZStd::to_string(numDisabledTests).c_str())));

                // Total duration of all tests in the suite
                testSuiteNode->append_attribute(doc.allocate_attribute(
                    GTest::Keys[GTest::TimeKey], doc.allocate_string(AZStd::to_string(testSuite.m_duration.count() / 1000.f).c_str())));

                // Number of errors in test suite (not supported in TestRun)
                testSuiteNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::ErrorsKey], "0"));

                // Individual tests in test suite
                for (const auto& testCase : testSuite.m_tests)
                {
                    AZ::rapidxml::xml_node<>* testCaseNode =
                        doc.allocate_node(AZ::rapidxml::node_element, GTest::Keys[GTest::TestCaseKey]);

                    // Name of test case
                    testCaseNode->append_attribute(
                        doc.allocate_attribute(TestRunFields::Keys[TestRunFields::NameKey], testCase.m_name.c_str()));

                    // Status of test case run
                    testCaseNode->append_attribute(doc.allocate_attribute(
                        GTest::Keys[GTest::TimeKey], doc.allocate_string(testCase.m_status == TestRunStatus::Run ? "run" : "notrun")));

                    // Total duration of test case run
                    testCaseNode->append_attribute(doc.allocate_attribute(
                        GTest::Keys[GTest::TimeKey], doc.allocate_string(AZStd::to_string(testCase.m_duration.count() / 1000.f).c_str())));

                    // Name of parent test suite
                    testCaseNode->append_attribute(
                        doc.allocate_attribute(GTest::Keys[GTest::ClassNameKey], testSuite.m_name.c_str()));

                    // Test failure message (TestRun doesn't store this message string)
                    if (testCase.m_result.has_value() && testCase.m_result.value() == TestRunResult::Failed)
                    {
                        AZ::rapidxml::xml_node<>* testCaseFailureNode =
                            doc.allocate_node(AZ::rapidxml::node_element, GTest::Keys[GTest::FailureKey]);

                        // Failure message
                        testCaseFailureNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::MessageKey], "Test failed (check log output for more details)"));

                        // Failure type (not supported by GTest)
                        testCaseFailureNode->append_attribute(doc.allocate_attribute(GTest::Keys[GTest::FailureTypeKey], ""));

                        testCaseNode->append_node(testCaseFailureNode);
                    }

                    testSuiteNode->append_node(testCaseNode);
                }

                rootNode->append_node(testSuiteNode);
            }

            AZStd::string xmlString;
            AZ::rapidxml::print(std::back_inserter(xmlString), doc);
            doc.clear();

            return xmlString;
        }
    } // namespace GTest

    AZStd::string SerializeTestRun(const TestRun& testRun)
    {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        // Run
        writer.StartObject();

        // Run duration
        writer.Key(TestRunFields::Keys[TestRunFields::DurationKey]);
        writer.Uint(static_cast<unsigned int>(testRun.GetDuration().count()));

        // Suites
        writer.Key(TestRunFields::Keys[TestRunFields::SuitesKey]);
        writer.StartArray();

        for (const auto& suite : testRun.GetTestSuites())
        {
            // Suite
            writer.StartObject();

            // Suite name
            writer.Key(TestRunFields::Keys[TestRunFields::NameKey]);
            writer.String(suite.m_name.c_str());

            // Suite duration
            writer.Key(TestRunFields::Keys[TestRunFields::DurationKey]);
            writer.Uint(static_cast<unsigned int>(suite.m_duration.count()));

            // Suite enabled
            writer.Key(TestRunFields::Keys[TestRunFields::EnabledKey]);
            writer.Bool(suite.m_enabled);

            // Suite tests
            writer.Key(TestRunFields::Keys[TestRunFields::TestsKey]);
            writer.StartArray();
            for (const auto& test : suite.m_tests)
            {
                // Test
                writer.StartObject();

                // Test name
                writer.Key(TestRunFields::Keys[TestRunFields::NameKey]);
                writer.String(test.m_name.c_str());

                // Test enabled
                writer.Key(TestRunFields::Keys[TestRunFields::EnabledKey]);
                writer.Bool(test.m_enabled);

                // Test duration
                writer.Key(TestRunFields::Keys[TestRunFields::DurationKey]);
                writer.Uint(static_cast<unsigned int>(test.m_duration.count()));

                // Test status
                writer.Key(TestRunFields::Keys[TestRunFields::StatusKey]);
                writer.Bool(static_cast<bool>(test.m_status));

                // Test result
                if (test.m_status == TestRunStatus::Run)
                {
                    writer.Key(TestRunFields::Keys[TestRunFields::ResultKey]);
                    writer.Bool(static_cast<size_t>(test.m_result.value()));
                }
                else
                {
                    writer.Key(TestRunFields::Keys[TestRunFields::ResultKey]);
                    writer.Null();
                }

                // End test
                writer.EndObject();
            }

            // End tests
            writer.EndArray();

            // End suite
            writer.EndObject();
        }

        // End suites
        writer.EndArray();

        // End run
        writer.EndObject();

        return stringBuffer.GetString();
    }

    TestRun DeserializeTestRun(const AZStd::string& testEnumString)
    {
        AZStd::vector<TestRunSuite> testSuites;
        rapidjson::Document doc;

        if (doc.Parse<0>(testEnumString.c_str()).HasParseError())
        {
            throw TestRunnerException("Could not parse enumeration data");
        }

        // Run duration
        const AZStd::chrono::milliseconds runDuration = AZStd::chrono::milliseconds{doc[TestRunFields::Keys[TestRunFields::DurationKey]].GetUint()};

        // Suites
        for (const auto& suite : doc[TestRunFields::Keys[TestRunFields::SuitesKey]].GetArray())
        {
            // Suite enabled
            testSuites.emplace_back(TestRunSuite{
                TestSuite<TestRunCase>{
                    suite[TestRunFields::Keys[TestRunFields::NameKey]].GetString(),
                    suite[TestRunFields::Keys[TestRunFields::EnabledKey]].GetBool()
                },
                AZStd::chrono::milliseconds{ suite[TestRunFields::Keys[TestRunFields::DurationKey]].GetUint() }
            });

            // Suite tests
            for (const auto& test : suite[TestRunFields::Keys[TestRunFields::TestsKey]].GetArray())
            {
                AZStd::optional<TestRunResult> result;
                TestRunStatus status = static_cast<TestRunStatus>(test[TestRunFields::Keys[TestRunFields::StatusKey]].GetBool());
                if (status == TestRunStatus::Run)
                {
                    result = static_cast<TestRunResult>(test[TestRunFields::Keys[TestRunFields::ResultKey]].GetBool());
                }
                const AZStd::chrono::milliseconds testDuration = AZStd::chrono::milliseconds{test[TestRunFields::Keys[TestRunFields::DurationKey]].GetUint()};
                testSuites.back().m_tests.emplace_back(
                    TestRunCase{
                        TestCase{
                            test[TestRunFields::Keys[TestRunFields::NameKey]].GetString(),
                            test[TestRunFields::Keys[TestRunFields::EnabledKey]].GetBool()
                        },
                         result, testDuration, status
                    });
            }
        }

        return TestRun(std::move(testSuites), runDuration);
    }
} // namespace TestImpact
