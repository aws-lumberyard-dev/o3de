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
* Class meant to be inherited by all C++ tests, connects/disconnects to TestCaseRequestBus
*
*/
#pragma once
 
#include <AzCore/Component/Component.h>
#include <TestHarness/TestCaseRequestBus.h>

namespace TestHarness
{
    // Boilerplate Macros -------------------------------------------------------------

    // Built-in Required Bus interaction for every test
    #define TH_SETUP_COMPLETED                                      TestHarness::TestCaseEventBus::Event(AZ::Component::GetEntityId(), &TestHarness::TestCaseEvent::SetupCompleted);
    #define TH_COMPLETE_TEST                                        TestHarness::TestCaseEventBus::Event(AZ::Component::GetEntityId(), &TestHarness::TestCaseEvent::Completed);
    #define TH_EXPECT_TRUE(_truthStatement)                         TestHarness::TestCaseEventBus::Event(AZ::Component::GetEntityId(), &TestHarness::TestCaseEvent::ExpectTrue, _truthStatement);

    // Macros to help users fill out the Reflect function more easily
    #define TH_ADD_EDITOR_PROPERTY(_property, _name, _description)  ->DataElement(AZ::Edit::UIHandlers::Default, _property, _name, _description)
    #define TH_ADD_FIELD(_name, _property)                          ->Field(_name, _property)
    #define TH_CONTEXT_EDIT                                         AZ::EditContext* edit = serialize->GetEditContext()
    #define TH_CONTEXT_SERIALIZE                                    AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context)
    #define TH_SERIALIZE(_class)                                    serialize->Class<_class, AZ::Component>()->Version(0)
    #define TH_REFLECT(_class, _name, _description)                 edit->Class<_class>(_name, _description) \
                                                                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "") \
                                                                    ->Attribute(AZ::Edit::Attributes::Category, "Test") \
                                                                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/ColliderSphere.png") \
                                                                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/ColliderSphere.png") \
                                                                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game")) \
                                                                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true) \
                                                                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly"))

    // ---------------------------------------------------------------------------------

    class TestCase
        : public TestCaseRequestBus::Handler
    {
    public:

        void Activate(AZ::EntityId id)
        {
            TestCaseRequestBus::Handler::BusConnect(id);
        }

        void Deactivate()
        {
            TestCaseRequestBus::Handler::BusDisconnect();
        }

        virtual void Setup() = 0;
        virtual void TearDown() = 0;
        virtual void Execute() = 0;
    };
} //namespace TestHarness
