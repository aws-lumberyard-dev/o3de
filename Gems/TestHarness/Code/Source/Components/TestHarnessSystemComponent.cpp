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
#include "TestHarnessSystemComponent.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Components/ConsoleBus.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <chrono>
#include <ctime>
#include <ILevelSystem.h>

namespace TestHarness
{

    void TestHarnessSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestHarnessSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        ReflectRegistrationBus(context);
        ReflectTestCaseInfoBus(context);
        ReflectTestCaseRequestBus(context);
        ReflectTestCaseEventBus(context);
    }

    void TestHarnessSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TestHarnessSystemService"));
    }

    void TestHarnessSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("TestHarnessSystemService"));
    }

    void TestHarnessSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void TestHarnessSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    ////////////////////////////////////////////////////////////////////////
    // AZ::Component interface implementation
    ////////////////////////////////////////////////////////////////////////

    void TestHarnessSystemComponent::Init()
    {
        m_currentTest_Active = false;
        m_currentTest_Timer = 0.0f;
        m_currentTest_TimedOut = false;
        m_currentTest_Expects = 0;
        m_currentTest_SettingUp = false;
        m_currentTest_SetupSuccess = false;
        m_completedTests.clear();
        m_registeredTests.clear();
        m_run = 0;
        m_pass = 0;
        m_fail = 0;
        m_givenTestsCounter = 0;
    }

    void TestHarnessSystemComponent::Activate()
    {
        CrySystemEventBus::Handler::BusConnect();
        AzFramework::GameEntityContextEventBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        RegistrationBus::Handler::BusConnect(GetEntityId());
        // TestCaseEventBus is connected later in TestHarnessSystemComponent::SetupGivenTest()
    }

    void TestHarnessSystemComponent::Deactivate()
    {
        TestCaseEventBus::Handler::BusDisconnect();
        RegistrationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::GameEntityContextEventBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////
    // Static member variables definitions 
    ////////////////////////////////////////////////////////////////////////

    bool TestHarnessSystemComponent::m_readySignalReceived = false;    // Flag to mark that something has requestd a ready-state
    bool TestHarnessSystemComponent::m_gameEntitiesReady = false;
    bool TestHarnessSystemComponent::m_testNameReceived = false;               // Flag to mark that something has given us a test
    AZStd::string TestHarnessSystemComponent::m_currentTestName;        // The name of the currently running test

    ////////////////////////////////////////////////////////////////////////
    // CrySystemEventBus
    ////////////////////////////////////////////////////////////////////////

    void TestHarnessSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params)
    {
        (void)params;
#if !defined(AZ_MONOLITHIC_BUILD)
        /* 
        This module is linked dynamically, so the gEnv pointer must be manually set
        here. Because this function runs regardless of whether in game or editor
        mode, calling gEnv->IsEditor() is necessary to distinguish between game and 
        editor mode. The remote console commands are only registered in game mode
        for test execution purposes.
        */
        gEnv = system.GetGlobalEnvironment();
#endif // AZ_MONOLITHIC_BUILD
        if (gEnv && !gEnv->IsEditor())
        {
            RegisterCommands();
        }
    }


    void TestHarnessSystemComponent::OnCrySystemShutdown([[maybe_unused]] ISystem& system)
    {
        /*
        This module is linked dynamically, so the gEnv pointer should be manually unset
        here.
        */
#if !defined(AZ_MONOLITHIC_BUILD)
        gEnv = nullptr;
#endif // AZ_MONOLITHIC_BUILD
    }

    ////////////////////////////////////////////////////////////////////////
    // GameEntityContextBus
    ////////////////////////////////////////////////////////////////////////

    void TestHarnessSystemComponent::OnGameEntitiesStarted()
    {
        m_gameEntitiesReady = true;
    }

    ////////////////////////////////////////////////////////////////////////
    // TickBus
    ////////////////////////////////////////////////////////////////////////

    void TestHarnessSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        (void)time;
        if (m_readySignalReceived && m_gameEntitiesReady)
        {
            m_readySignalReceived = false;
            m_gameEntitiesReady = false;
            CollectRegisteredTests();
            CreateLevelReadySignal();
        }
        if (m_testNameReceived)
        {
            m_testNameReceived = false;
            m_givenTestsCounter++;
            UpdateTestRunnerLogNode();
            SetupGivenTest();
        }
        if (m_currentTest_SettingUp)
        {
            m_currentTest_Timer += deltaTime;
            if (m_currentTest_SetupSuccess)
            {
                m_currentTest_Active = true;
                m_currentTest_SettingUp = false;
                m_currentTest_SetupSuccess = false;
                TestCaseRequestBus::Event(m_currentTest->GetId(), &TestCaseRequest::Execute);

            }
            else if (m_currentTest_Timer >= m_secondsTestRunnerTimeout)
            {
                AZ_TracePrintf("TestHarnessSystemComponent", "Setup timed out.");
                m_currentTest_TimedOut = true;
                Completed();
            }
        }
        if (m_currentTest_Active)
        {
            m_currentTest_Timer += deltaTime;
            if (m_currentTest_Timer >= m_secondsTestRunnerTimeout)
            {   // kill-off a test taking too long
                m_currentTest_TimedOut = true;
                Completed();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // TestHarness::TestCaseEventBus
    ////////////////////////////////////////////////////////////////////////

    void TestHarnessSystemComponent::SetupCompleted()
    {
        // tell OnTick to call Execute on the next tick
        m_currentTest_SetupSuccess = true;
    }

    void TestHarnessSystemComponent::ExpectTrue(bool actual)
    {
        // this allows the testcases to call "assert" and for us to log if it passed without crashing/stopping
        if (!actual)
        {
            m_currentTest_Expects++;
        }
    }

    void TestHarnessSystemComponent::Completed()
    {
        bool passed = false;
        if (m_currentTest_TimedOut || (m_currentTest_Expects > 0))
        {
            m_fail++;
        }
        else
        {
            passed = true;
            m_pass++;
        }

        TestCaseRequestBus::Event(m_currentTest->GetId(), &TestCaseRequest::TearDown);
        TestCaseEventBus::Handler::BusDisconnect();

        m_completedTests.insert(m_currentTest->GetId());
        UpdateTestRunnerLogNode();
        WriteTestReportNode(passed);

        m_currentTest = nullptr;
        m_currentTestName.clear();
        m_currentTest_Active = false;
        m_currentTest_Timer = 0.0f;
        m_currentTest_TimedOut = false;
        m_currentTest_Expects = 0;
    }

    ////////////////////////////////////////////////////////////////////////
    // TestHarness::RegistrationBus
    ////////////////////////////////////////////////////////////////////////

    AZStd::vector<AZ::EntityId> TestHarnessSystemComponent::GetAllTestCases()
    {
        AZStd::vector<AZ::EntityId> testCases;
        TestCaseInfoBus::EnumerateHandlers([&testCases](TestCaseInfo* testCaseInfo)
        {
            AZ::EntityId id = testCaseInfo->GetEntityId();
            if (id.IsValid())
            {
                testCases.push_back(id);
            }
            return true;
        });
        return testCases;
    }

    ////////////////////////////////////////////////////////////////////////
    // Helper functions
    ////////////////////////////////////////////////////////////////////////

    void TestHarnessSystemComponent::RegisterCommands()
    {
        // Registering the commands that the PyTest plugin will send over:
        //  - th_Ready      :   Wait until all tests are ready and create JSON file as "ready signal"
        //  - th_RunTest    :   Run the test given as an argument
            REGISTER_COMMAND("th_Ready", &ReadyCommand, VF_EXPERIMENTAL, "ready flag set by in-engine Test Harness");
            REGISTER_COMMAND("th_RunTest", &ParseTestNameFromCommand, VF_EXPERIMENTAL, "run the test given as an argument");
    }

    void TestHarnessSystemComponent::ReadyCommand(IConsoleCmdArgs* p_args)
    {
        (void)p_args;
        m_readySignalReceived = true;
    }

    void TestHarnessSystemComponent::ParseTestNameFromCommand(IConsoleCmdArgs* p_args)
    {
        if (p_args->GetArgCount() != 2)
        {
            AZ_TracePrintf("TestHarnessSystemComponent", "th_RunTest requires an argument of the test name to run (none was provided).");
            return;
        }
        else
        {
            AZStd::string testName = p_args->GetArg(1);
            m_currentTestName = testName;
            m_testNameReceived = true;
        }
    }

    void TestHarnessSystemComponent::CollectRegisteredTests()
    {
        m_registeredTests.clear();
        AZStd::vector<AZ::EntityId> registeredTests = GetAllTestCases();

        for (AZ::EntityId currentEntity : registeredTests)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, currentEntity);
            if (!entity)
            {
                AZ_TracePrintf("TestHarnessSystemComponent", "Warning: Entity was removed after initial query of tests.");
                continue;
            }
            AZStd::pair<AZStd::string, AZ::EntityId> newTest(GetTestCaseName(currentEntity), currentEntity);
            m_registeredTests.insert(newTest);
        }
    }

    void TestHarnessSystemComponent::CreateLevelReadySignal()
    {
        ClearTestData();

        // Creating the ready signal JSON file to be read by Python to confirm the level is ready
        rapidjson::Document::AllocatorType& testDataAllocator = m_testData.GetAllocator();
        rapidjson::Value readySignal(rapidjson::kObjectType);
        
        // get the time since epoch in milliseconds, and use count() to get an integer instead of a std::chrono object because AddMember doesn't accept that as a type
        uint64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        // get the number of registered tests collected in game mode
        size_t registeredTestsListSize = m_registeredTests.size(); //TODO need to cast to uint?

        readySignal.AddMember("ready_timestamp", milliseconds, testDataAllocator);
        readySignal.AddMember("number_of_registered_tests_collected_in_level", registeredTestsListSize, testDataAllocator);
        m_testData.AddMember("LevelReadySignal", readySignal, testDataAllocator);

        AZ::JsonSerializationUtils::WriteJsonFile(m_testData, CreateLogPath(AZStd::string("testharness_execution.json")));
    }

    void TestHarnessSystemComponent::SetupGivenTest()
    {
        AZ::Entity* testEntity = GetEntityFromCommandName(m_currentTestName);
        m_currentTest = testEntity;
        m_run++;
        if (!testEntity)
        {
            // tests that are requested to be run but whose entities are not found in the level are considered 
            // to be tests that have run and that have failed.
            AZ_TracePrintf("TestHarnessSystemComponent", "A valid test entity was not found in the game level.");
            m_fail++;
            WriteTestReportNode(false, true);
            UpdateTestRunnerLogNode();
        }
        else
        {
            m_currentTest_SettingUp = true; // tell OnTick to wait for m_currentTest_SetupSuccess
            AZ::EntityId testID = testEntity->GetId();
            TestCaseEventBus::Handler::BusConnect(testID);
            TestCaseRequestBus::Event(testID, &TestCaseRequest::Setup);
        }
    }

    AZ::Entity* TestHarnessSystemComponent::GetEntityFromCommandName(AZStd::string testName)
    {
        if (m_registeredTests.count(testName) == 1 && m_registeredTests[testName].IsValid())
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_registeredTests[testName]);
            if (entity)
            {
                return entity;
            }
            else
            {
                AZ_TracePrintf("TestHarnessSystemComponent", "No valid entity is currently registered with the name '%s'.", testName.c_str());
            }
        }
        else
        {
            // Don't forget to export level to engine in the Editor
            AZ_TracePrintf("TestHarnessSystemComponent", "No test has been registered with the name '%s'.", testName.c_str());
        }
        return nullptr;
    }

    AZStd::string TestHarnessSystemComponent::GetTestCaseName(AZ::EntityId registeredTest)
    {
        AZStd::string testName = "";
        TestCaseInfoBus::EventResult(testName, registeredTest, &TestCaseInfo::GetTestCaseName);
        return testName;
    }

    void TestHarnessSystemComponent::UpdateTestRunnerLogNode()
    {
        if (m_testData.HasMember("TestRunnerLog"))
        {   // already exists, update the node
            m_testData["TestRunnerLog"]["ran"] = m_run;
            m_testData["TestRunnerLog"]["passed"] = m_pass;
            m_testData["TestRunnerLog"]["failed"] = m_fail;
        }
        else
        {   // doesn't exist, create a new node
            rapidjson::Document::AllocatorType& testDataAllocator = m_testData.GetAllocator();
            rapidjson::Value testRunnerLog(rapidjson::kObjectType);
            testRunnerLog.AddMember("ran", m_run, testDataAllocator);
            testRunnerLog.AddMember("passed", m_pass, testDataAllocator);
            testRunnerLog.AddMember("failed", m_fail, testDataAllocator);
            m_testData.AddMember("TestRunnerLog", testRunnerLog, testDataAllocator);
        }
        AZ::JsonSerializationUtils::WriteJsonFile(m_testData, CreateLogPath(AZStd::string("testharness_execution.json")));
    }

    void TestHarnessSystemComponent::WriteTestReportNode(bool passed, bool missing)
    {
        rapidjson::Document::AllocatorType& testDataAllocator = m_testData.GetAllocator();
        rapidjson::Value testReport(rapidjson::kObjectType);

        // get the time since epoch in milliseconds, and use count() to get an integer instead of a std::chrono object because AddMember doesn't accept that as a type
        uint64_t milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        testReport.AddMember("result_timestamp", milliseconds, testDataAllocator);

        testReport.AddMember("pass", passed, testDataAllocator);
        testReport.AddMember("missing", missing, testDataAllocator);
        testReport.AddMember("timeout", m_currentTest_TimedOut, testDataAllocator);
        testReport.AddMember("asserts", m_currentTest_Expects, testDataAllocator);
        AZStd::string timerString = AZStd::string::format("%f seconds", m_currentTest_Timer);
        rapidjson::Value timer(timerString.c_str(), testDataAllocator);
        testReport.AddMember("timer", timer, testDataAllocator);

        // this will format the node name to something like: "Test-1_SanityTest" 
        // for the case when multiple tests are run in the same level instance
        AZStd::string entityName = AZStd::string::format("Test-%d_%s", m_givenTestsCounter, m_currentTestName.c_str());
        rapidjson::Value testName(entityName.c_str(), testDataAllocator);
        m_testData.AddMember(testName, testReport, testDataAllocator);
        AZ::JsonSerializationUtils::WriteJsonFile(m_testData, CreateLogPath(AZStd::string("testharness_execution.json")));
    }

    AZStd::string TestHarnessSystemComponent::CreateLogPath(const AZStd::string& filename)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        char resolvedPath[AZ::IO::MaxPathLength] = { '\0' };
        const AZStd::string fullFilePath = AZStd::string::format("@log@/TestHarnessLogs/%s", filename.c_str());
        if (fileIO->ResolvePath(fullFilePath.c_str(), resolvedPath, AZ_ARRAY_SIZE(resolvedPath)))
        {
            return AZStd::string(resolvedPath);
        }
        return AZStd::string();
    }

    void TestHarnessSystemComponent::ClearTestData()
    {
        m_testData.SetObject(); // reseting the current test's data
        m_currentTest = nullptr;
        m_currentTestName.clear(); // clearing out all internal data
        m_currentTest_Active = false;
        m_currentTest_Timer = 0.0f;
        m_currentTest_TimedOut = false;
        m_currentTest_Expects = 0;
        m_currentTest_SettingUp = false;
        m_currentTest_SetupSuccess = false;
        m_completedTests.clear();
        m_run = 0;
        m_pass = 0;
        m_fail = 0;
        m_givenTestsCounter = 0;
    }

}
