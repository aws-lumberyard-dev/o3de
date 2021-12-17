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

#include "TestHarness/TestCaseInfoBus.h"
#include "TestHarness/TestCaseEventBus.h"
#include "TestHarness/TestCaseRequestBus.h"
#include <AzCore/Component/Component.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace TestHarness
{
    class TestCaseConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(TestCaseConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(TestCaseConfig, "{27B625DC-FABB-4E12-8A52-C4C18CE0DC60}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        TestCaseConfig();
        virtual ~TestCaseConfig() = default;
        AZStd::string m_testCaseName;
        bool m_enabled;
        AZStd::set<AZStd::string> m_tags;
    };

    class TestCaseComponent
        : public AZ::Component
        , public TestCaseInfoBus::Handler
        , public TestCaseEventBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(TestCaseComponent, "{85CB5D26-860B-4E83-B631-12DCD7365100}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);

        TestCaseComponent(const TestCaseConfig& configuration);
        TestCaseComponent() = default;
        ~TestCaseComponent() = default;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        //////////////////////////////////////////////////////////////////////////
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfiguration) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfiguration)  const override;

        //////////////////////////////////////////////////////////////////////////
        // TestHarness::TestCaseInfoBus
        //////////////////////////////////////////////////////////////////////////
        AZ::EntityId GetEntityId() override;
        AZStd::string GetTestCaseName() override;
        bool IsEnabled() override;
        AZStd::set<AZStd::string> GetUserDefinedTags() override;

        //////////////////////////////////////////////////////////////////////////
        //  TestHarness::TestCaseEventBus
        //////////////////////////////////////////////////////////////////////////
        void Completed() override;

    private:
        TestCaseConfig m_configuration;

    };
}
