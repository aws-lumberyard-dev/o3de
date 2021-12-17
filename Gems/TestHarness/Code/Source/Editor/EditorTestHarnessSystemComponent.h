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
#include <AzCore/Component/Component.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace TestHarness
{
    class EditorTestHarnessSystemComponent
        : public AzToolsFramework::Components::EditorComponentBase // specific to editor module
        , public RegistrationBus::Handler
        , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorTestHarnessSystemComponent, "{72D4CE01-6C87-4AAD-81D3-82A30CF648EB}"); // specific to editor module

        ///! Exposes the component to the engine by reflection of various contexts.
        static void Reflect(AZ::ReflectContext* context);
        ///! Puts the name of the service this component provides.
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        ///! Ensures that this component is not attached to an entity with an incompatible component attached already.
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        ///! Determines the prerequisite components that need to be there for this service to work.
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        ////////////////////////////////////////////////////////////////////////
        ///! Overrides the component init function.
        void Init() override;
        ///! Overrides the component activate function. Used to connect to required ebus handlers.
        void Activate() override;
        ///! Overrides the component deactivate function. Used to disconnect from required ebus handlers.
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::ToolsApplicationEvents::Bus
        ////////////////////////////////////////////////////////////////////////
        ///! Invoked when saving (i.e. by pressing CTRL-S) in the editor.
        void OnSaveLevel() override;

        ////////////////////////////////////////////////////////////////////////
        // TestHarness::RegistrationBus
        ////////////////////////////////////////////////////////////////////////
        ///! Gets a vector of all registered test case entityIds.
        AZStd::vector<AZ::EntityId> GetAllTestCases() override;
    
    private:
        ////////////////////////////////////////////////////////////////////////
        // Helper functions
        ////////////////////////////////////////////////////////////////////////
        ///! Writes test data (test name, entity ID, is enabled or not, level name,
        ///! project name) into testharness_collection.json.
        ///! @param registeredTests: entity IDs of tests in a level opened in the Editor.
        void WriteTestDataToFile(const AZStd::vector<AZ::EntityId>& registeredTests);
        ///! Stores test data (test name, entity ID, is enabled or not) in rapidjson::Value& tests.
        ///! @param tests: where test data are collected and stored.
        ///! @param registeredTests: entity IDs of tests in a level opened in the Editor.
        ///! @param testDataAllocator: rapidjson's memory allocator object.
        void CollectTestInfo(rapidjson::Value& tests, const AZStd::vector<AZ::EntityId>& registeredTests, rapidjson::Document::AllocatorType& testDataAllocator);
        ///! Gets given test case's name.
        ///! @param registeredTest: the entity ID of the a test in a level opened in the Editor.
        AZStd::string GetTestCaseName(AZ::EntityId registeredTest);
        ///! Gets boolean value of whether the given test is enabled.
        ///! @param registeredTest: the entity ID of the a test in a level opened in the Editor.
        bool GetTestEnabledStatus(AZ::EntityId registeredTest);
        ///! Gets tags associated with the given test. A test will always have associated level and project names added as tags.
        ///! @param registeredTest: the entity ID of the a test in a level opened in the Editor.
        AZStd::string GetTagsAsString(AZ::EntityId registeredTest);
        ///! Gets level name from IEditor.
        AZStd::string GetLevelName();
        ///! Gets project name from IEditor.
        AZStd::string GetProjectName();
        ///! Resolve the output path of logs.
        ///! @param filename: the name of the test log.
        static AZStd::string CreateLogPath(const AZStd::string& filename);

    };
} // namespace TestHarness
