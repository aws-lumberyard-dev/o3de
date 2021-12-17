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
#pragma once

#include "TestHarness/RegistrationBus.h"
#include "TestHarness/TestCaseInfoBus.h"
#include "TestHarness/TestCaseEventBus.h"
#include "TestHarness/TestCaseRequestBus.h"
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <ISystem.h>
#include <IConsole.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

namespace TestHarness
{
    class TestHarnessSystemComponent
        : public AZ::Component
        , public TestCaseEventBus::Handler
        , public AZ::TickBus::Handler
        , public RegistrationBus::Handler
        , private CrySystemEventBus::Handler
        , private AzFramework::GameEntityContextEventBus::Handler
    {
    public:
        AZ_COMPONENT(TestHarnessSystemComponent, "{D308CFF0-5C86-4478-A169-051AC17B4E78}"); 

        TestHarnessSystemComponent() : m_testData(rapidjson::kObjectType), m_secondsTestRunnerTimeout(10.0f)
        {
        }

        ///! Exposes the component to the engine by reflection of various contexts.
        static void Reflect(AZ::ReflectContext* context);
        ///! Puts the name of the service this component provides.
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        ///! Ensures that this component is not attached to an entity with an incompatible component attached already.
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        ///! Determines the prerequisite components that need to be there for this service to work.
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        ///! Determines and reports the components that require the test harness component.
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
    
    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        ////////////////////////////////////////////////////////////////////////
        ///! Overrides the component required init function.
        void Init() override;
        ///! Overrides the component activate command. Used to connect to required ebus handlers.
        void Activate() override;
        ///! Overrides the component deactivate command. Used to disconnect from required ebus handlers.
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // CrySystemEventBus
        ////////////////////////////////////////////////////////////////////////
        ///! Sets gEnv if module is linked dynamically, registers remote console commands if in game mode.
        ///! @param system: reference to ISystem.
        ///! @param params: reference to SSystemInitParams.
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& params) override;
        ///! Cleans up gEnv if module is linked dynamically.
        ///! @param system: reference to ISystem.
        void OnCrySystemShutdown(ISystem& system) override;


        ////////////////////////////////////////////////////////////////////////
        // GameEntityContextBus
        ////////////////////////////////////////////////////////////////////////
        ///! Signals when the game entity context is loaded and activated, after a level has been loaded.
        ///! If the concept of levels is eradicated, this event needs to be replaced.
        void OnGameEntitiesStarted() override;

        ////////////////////////////////////////////////////////////////////////
        // TickBus
        ////////////////////////////////////////////////////////////////////////
        ///! Manages test execution based on system readiness.
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        ////////////////////////////////////////////////////////////////////////
        // TestHarness::TestCaseEventBus
        ////////////////////////////////////////////////////////////////////////
        ///! Handles when the test completes its setup.
        void SetupCompleted() override;
        ///! Handles the test asserts.
        ///! @param actual: the result of the test's assert.
        void ExpectTrue(bool actual) override;
        ///! Handles when a test completes.
        void Completed() override;

        ////////////////////////////////////////////////////////////////////////
        // TestHarness::RegistrationBus
        ////////////////////////////////////////////////////////////////////////
        ///! Gets a vector of all registered test case entityIds.
        AZStd::vector<AZ::EntityId> GetAllTestCases() override;

    private:
        ////////////////////////////////////////////////////////////////////////
        // Helper functions
        ////////////////////////////////////////////////////////////////////////
        ///! Registers the remote console commands to be called from TestHarnessLauncher.py.
        static void RegisterCommands();
        ///! Function called by the "th_Ready" remote console command.
        ///! @params p_args: arguments passed in with command.
        static void ReadyCommand(IConsoleCmdArgs* p_args);
        ///! Function called by the "th_RunTest" remote console command.
        ///! @params p_args: arguments passed in with command.
        static void ParseTestNameFromCommand(IConsoleCmdArgs* p_args);
        ///!
        void CollectRegisteredTests();
        ///! Creates the json file the level ready signal is added to.
        void CreateLevelReadySignal();
        ///! Function for setting up initial test data and sending Setup event
        void SetupGivenTest();
        ///! Gets an entity from its entity ID stored as a value of the test name, which
        ///! is given from Python in a remote console command
        ///! @param entityName: string of the entity's name
        AZ::Entity* GetEntityFromCommandName(AZStd::string entityName);
        ///! Gets given test case's name.
        ///! @param registeredTest: the entity ID of the a test in a level opened in the Editor.
        AZStd::string GetTestCaseName(AZ::EntityId registeredTest);
        ///! Updates the Totals test runner node with the current stored info
        void UpdateTestRunnerLogNode();
        ///! Creates a new node in the log of the test that was just run
        ///! @param passed: whether or not the test passed
        ///! @param missing: whether or not the test was missing (default is false)
        void WriteTestReportNode(bool passed, bool missing = false);
        ///! Resolve the output path of the log.
        ///! @param filename: The name of the test log.
        static AZStd::string CreateLogPath(const AZStd::string& filename);
        ///! Resets the state of the document.
        void ClearTestData();

        rapidjson::Document m_testData;         ///< The location of data stored during the test run.

        float m_secondsTestRunnerTimeout;       ///< The number of seconds of our test runner's timeout (defaults to 10 seconds)

        static bool m_readySignalReceived;      ///< Flag to mark that something has requested a ready-state
        static bool m_gameEntitiesReady;        ///< Flag to mark that game entites are loaded into the level and ready
        static bool m_testNameReceived;         ///< Flag to mark that something has given us a test

        AZ::Entity* m_currentTest;              ///< The current test we are running
        static AZStd::string m_currentTestName; ///< The name of the currently running test

        bool m_currentTest_Active;              ///< Flag if the current test is active
        float m_currentTest_Timer;              ///< The timer of the currently running test
        bool m_currentTest_TimedOut;            ///< Flag for if the current test timed out
        int m_currentTest_Expects;              ///< The number of expects the test failed
        bool m_currentTest_SettingUp;           ///< Flag for if the currently running test is setting up
        bool m_currentTest_SetupSuccess;        ///< Flag for if the currently running test has finished setting up

        AZStd::map<AZStd::string, AZ::EntityId, AZStd::less<AZStd::string>, AZ::OSStdAllocator> m_registeredTests;  ///< container storing all registered tests
        AZStd::set<AZ::EntityId> m_completedTests;  ///< container storing all tests that have been completed              
        
        int m_run;                              ///< The number of tests that have currently run
        int m_pass;                             ///< The number of tests that have currently passed
        int m_fail;                             ///< The number of tests that have currently failed 
                                                ///< (a test can fail at setup because the test entity wasn't found in the level, 
                                                ///< or at completion because the test timed out or an assertion failed).
        int m_givenTestsCounter;                ///< The total number of tests given to us

    };
}
