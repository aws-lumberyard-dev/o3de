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
#include "TestCaseComponent.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace TestHarness
{
    TestCaseConfig::TestCaseConfig()
        : m_testCaseName(""), m_enabled(true), m_tags(AZStd::set<AZStd::string>())
    {
    }

    void TestCaseConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestCaseConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("Test Case Name", &TestCaseConfig::m_testCaseName)
                ->Field("Enabled", &TestCaseConfig::m_enabled)
                ->Field("Tags", &TestCaseConfig::m_tags)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<TestCaseConfig>("TestHarness Test Case Configuration", "Configuration for TestHarness Test Case component.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestCaseConfig::m_testCaseName, "Test Case Name", "Test cases are named here.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestCaseConfig::m_enabled, "Enabled", "Test case will be executed when enabled.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &TestCaseConfig::m_tags, "Tags", "Test cases may be filtered by user-defined or static tags. Static tags include level and project name and don't need to be added.")
                    ;
            }
        }

    }

    TestCaseComponent::TestCaseComponent(const TestCaseConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void TestCaseComponent::Reflect(AZ::ReflectContext* context)
    {
        TestCaseConfig::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TestCaseComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &TestCaseComponent::m_configuration)
                ;
        }
    }

    void TestCaseComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("TestCaseService"));
    }

    void TestCaseComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        /*
        Currently only one test case component can be attached to an entity. This may be changed in the future
        if test names are not taken from entity names and if test case components and C++/Lua test script 
        components are grouped into one component.
        */
        incompatible.push_back(AZ_CRC("TestCaseService")); 
    }

    void TestCaseComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService"));
    }

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component interface implementation
    //////////////////////////////////////////////////////////////////////////

    void TestCaseComponent::Init()
    {
    }

    void TestCaseComponent::Activate()
    {
        const AZ::EntityId entityId = AZ::Component::GetEntityId();
        TestCaseInfoBus::Handler::BusConnect(entityId);
        TestCaseEventBus::Handler::BusConnect(entityId);
    }

    void TestCaseComponent::Deactivate()
    {
        TestCaseInfoBus::Handler::BusDisconnect();
        TestCaseEventBus::Handler::BusDisconnect();
    }

    bool TestCaseComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const TestCaseConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool TestCaseComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<TestCaseConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    // TestHarness::TestCaseInfoBus
    //////////////////////////////////////////////////////////////////////////

    AZ::EntityId TestCaseComponent::GetEntityId()
    {
        return AZ::Component::GetEntityId();
    }

    AZStd::string TestCaseComponent::GetTestCaseName()
    {
        return m_configuration.m_testCaseName;
    }

    bool TestCaseComponent::IsEnabled()
    {
        return m_configuration.m_enabled;
    }
    
    AZStd::set<AZStd::string> TestCaseComponent::GetUserDefinedTags()
    {
        return m_configuration.m_tags;
    }

    //////////////////////////////////////////////////////////////////////////
    //  TestHarness::TestCaseEventBus
    //////////////////////////////////////////////////////////////////////////

    void TestCaseComponent::Completed()
    {
        TestCaseInfoBus::Handler::BusDisconnect();
        TestCaseEventBus::Handler::BusDisconnect();
    }
}
