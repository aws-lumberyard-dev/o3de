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

#include "EditorTestHarnessSystemComponent.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
//TODO desires access to editor
//#include <IEditor.h>

namespace TestHarness
{
    
    void EditorTestHarnessSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorTestHarnessSystemComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(0)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorTestHarnessSystemComponent>("Editor TestHarness System", "Manages discovery and registration of test case components")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }

            ReflectRegistrationBus(context);
            ReflectTestCaseInfoBus(context);
        }
    }

    void EditorTestHarnessSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("EditorTestHarnessSystemService"));
    }

    void EditorTestHarnessSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("EditorTestHarnessSystemService"));
    }

    void EditorTestHarnessSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TestHarnessSystemService"));
    }

    ////////////////////////////////////////////////////////////////////////
    // AZ::Component interface implementation
    ////////////////////////////////////////////////////////////////////////

    void EditorTestHarnessSystemComponent::Init()
    {
        AzToolsFramework::Components::EditorComponentBase::Init();
    }

    void EditorTestHarnessSystemComponent::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        RegistrationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
    }

    void EditorTestHarnessSystemComponent::Deactivate()
    {
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        RegistrationBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::ToolsApplicationEvents::Bus
    ////////////////////////////////////////////////////////////////////////

    void EditorTestHarnessSystemComponent::OnSaveLevel()
    {
        AZStd::vector<AZ::EntityId> registeredTests = GetAllTestCases();
        WriteTestDataToFile(registeredTests);
    }

    ////////////////////////////////////////////////////////////////////////
    // TestHarness::RegistrationBus
    ////////////////////////////////////////////////////////////////////////

    AZStd::vector<AZ::EntityId> EditorTestHarnessSystemComponent::GetAllTestCases()
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

    void EditorTestHarnessSystemComponent::WriteTestDataToFile(const AZStd::vector<AZ::EntityId>& registeredTests)
    {
        /*
        Test names and associated information (the test's entity ID and whether the test is enabled or not)
        are collected anew each time this function is invoked, because it is simpler than updating tests
        when test attributes are modified (i.e. test name is changed, enabled option is changed), which
        theoretically may be done by relying on entity ID to identify a test but which may not be the
        best approach in consideration of potential future modifications to this gem where test cases
        perhaps might no longer be named by entity name or identified by entity ID.
        */
        AZStd::string fileName = CreateLogPath(AZStd::string("testharness_collection.json"));
        auto result = AZ::JsonSerializationUtils::ReadJsonFile(fileName);
        rapidjson::Value level(rapidjson::kObjectType);
        rapidjson::Value tests(rapidjson::kObjectType);
        AZStd::string registeredLevelName = GetLevelName();
        AZStd::string registeredProjectName = GetProjectName();
        if (result.IsSuccess())
        {
            rapidjson::Document& testDataDocument = result.GetValue(); // get value only if file exists
            rapidjson::Document::AllocatorType& testDataAllocator = testDataDocument.GetAllocator();
            CollectTestInfo(tests, registeredTests, testDataAllocator);

            if (testDataDocument.HasMember(registeredProjectName.c_str()))
            {
                if (testDataDocument[registeredProjectName.c_str()].HasMember(registeredLevelName.c_str()))
                {
                    // overwrite test information in existing level node in rapidjson DOM tree
                    testDataDocument[registeredProjectName.c_str()][registeredLevelName.c_str()] = tests;
                }
                else
                {
                    // add new level node to rapidjson DOM tree
                    testDataDocument[registeredProjectName.c_str()].AddMember(rapidjson::Value(registeredLevelName.c_str(), testDataAllocator).Move(), tests, testDataAllocator);
                }
            }
            else
            {
                level.AddMember(rapidjson::Value(registeredLevelName.c_str(), testDataAllocator).Move(), tests, testDataAllocator);
                testDataDocument.AddMember(rapidjson::Value(registeredProjectName.c_str(), testDataAllocator).Move(), level, testDataAllocator);
            }
            AZ::JsonSerializationUtils::WriteJsonFile(testDataDocument, fileName);
        }
        else
        {
            rapidjson::Document testDataDocument;
            testDataDocument.SetObject();
            rapidjson::Document::AllocatorType& testDataAllocator = testDataDocument.GetAllocator();
            CollectTestInfo(tests, registeredTests, testDataAllocator);
            level.AddMember(rapidjson::Value(registeredLevelName.c_str(), testDataAllocator).Move(), tests, testDataAllocator);
            testDataDocument.AddMember(rapidjson::Value(registeredProjectName.c_str(), testDataAllocator).Move(), level, testDataAllocator);
            AZ::JsonSerializationUtils::WriteJsonFile(testDataDocument, fileName);
        }
    }

    void EditorTestHarnessSystemComponent::CollectTestInfo(rapidjson::Value& testInfo, const AZStd::vector<AZ::EntityId>& registeredTests, rapidjson::Document::AllocatorType& testDataAllocator)
    {
        /*
        Writes test information (test name, entity ID, whether the test is enabled or not) into a rapidjson::Value.
        */
        AZStd::set<AZStd::string> testNames;
        for (AZ::EntityId registeredTest : registeredTests)
        {
            AZStd::string registeredTestName = GetTestCaseName(registeredTest);
            if (registeredTestName.empty())
            {
                AZ_Error("EditorTestHarnessSystemComponent", false, "Registered test name is empty, and will be ignored by TestHarness.");
                continue;
            }
            if (testNames.find(registeredTestName) != testNames.end())
            {
                AZStd::string errorMsg = "Registered test name " + registeredTestName + " is a duplicate, and will be ignored.";
                AZ_Error("EditorTestHarnessSystemComponent", false, errorMsg.c_str());
                continue;
            }
            testNames.insert(registeredTestName);
            rapidjson::Value testDetails(rapidjson::kObjectType);
            // rapidjson's move() copies the string into a string type accepted by rapidjson
            testDetails.AddMember("entity_id", rapidjson::Value(registeredTest.ToString().c_str(), testDataAllocator).Move(), testDataAllocator);
            testDetails.AddMember("is_enabled", GetTestEnabledStatus(registeredTest), testDataAllocator);
            testDetails.AddMember("tags", rapidjson::Value(GetTagsAsString(registeredTest).c_str(), testDataAllocator).Move(), testDataAllocator);
            testInfo.AddMember(rapidjson::Value(registeredTestName.c_str(), testDataAllocator).Move(), testDetails, testDataAllocator);
        }
    }

    AZStd::string EditorTestHarnessSystemComponent::GetTestCaseName(AZ::EntityId registeredTest)
    {
        /* 
        Test name is retrieved from the "TestHarness Test Case" component, which may be found attached to
        an entity in the Editor. Currently there must be no more than one active test case component per 
        entity, which is enforced in TestCaseComponent.cpp's GetIncompatibleServices().

        If a test name is not supplied by the user, the test will not be collected or written into 
        testharness_collection.json. 

        If there are duplicate test names in a level, they will be collected and written into 
        testharness_collection.json - on the game execution side, only one will be executed, because
        JSON ignores duplicate keys.
        */
        AZStd::string testName = "";
        TestCaseInfoBus::EventResult(testName, registeredTest, &TestCaseInfo::GetTestCaseName);
        return testName;
    }

    bool EditorTestHarnessSystemComponent::GetTestEnabledStatus(AZ::EntityId registeredTest)
    {
        bool enabled = false;
        TestCaseInfoBus::EventResult(enabled, registeredTest, &TestCaseInfo::IsEnabled);
        return enabled;
    }

    AZStd::string EditorTestHarnessSystemComponent::GetTagsAsString(AZ::EntityId registeredTest)
    {
        /* Tags retrieved include both static tags (level and project names associated with
           the test) and tags that users define in the Editor. */
        AZStd::set<AZStd::string> tags;
        TestCaseInfoBus::EventResult(tags, registeredTest, &TestCaseInfo::GetUserDefinedTags);
        AZStd::string result;
        AZStd::string comma = "";
        for (AZStd::string tag : tags)
        {
            result += comma + tag;
            comma = ",";
        }
        // if there are no user-defined tags, the variable comma is not set to ","
        result += comma + GetTestCaseName(registeredTest) + "," + GetLevelName() + "," + GetProjectName();
        return result;
    }

    AZStd::string EditorTestHarnessSystemComponent::GetLevelName()
    {
        //TODO from editor:
        //return AZStd::string(GetIEditor()->GetGameEngine()->GetLevelName().toUtf8().data());
        return AZStd::string("");
    }

    AZStd::string EditorTestHarnessSystemComponent::GetProjectName()
    {
        // TODO from editor:
        //return AZStd::string(GetIEditor()->GetAssetTagging()->GetProjectName().toUtf8().data());
        return AZStd::string("");
    }

    AZStd::string EditorTestHarnessSystemComponent::CreateLogPath(const AZStd::string& filename)
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
}
