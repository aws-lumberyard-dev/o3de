/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzFramework/Quality/QualitySystemComponent.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AzFramework;

    class QualitySystemComponentTestFixture : public LeakDetectionFixture
    {
    public:
        QualitySystemComponentTestFixture()
            : LeakDetectionFixture()
        {
        }

    protected:
        void SetUp() override
        {
            m_settingsRegistry = AZStd::make_unique<SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());

            m_console = AZStd::make_unique<AZ::Console>();
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());

            m_qualitySystemComponent = AZStd::make_unique<QualitySystemComponent>();
        }

        void TearDown() override
        {
            // reset the console first so all the variables are moved to deferred head
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();

            // next deactivate/reset to unlink from deferred head
            m_qualitySystemComponent->Deactivate();
            m_qualitySystemComponent.reset();


            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();
        }

        AZStd::unique_ptr<AZ::Console> m_console;
        AZStd::unique_ptr<QualitySystemComponent> m_qualitySystemComponent;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
    };

    TEST_F(QualitySystemComponentTestFixture, QualitySystem_Registers_Group_CVars)
    {
        m_settingsRegistry->MergeSettings(R"(
            {
                "O3DE": {
                    "Quality": {
                        "Groups": {
                            "q_test": {
                                "Levels": [ "low", "high" ],
                                "Default": 1,
                                "Description": "q_test quality group",
                                "Settings": {
                                    "q_test_sub": [0,1]
                                }
                            },
                            "q_test_sub": {
                                "Levels": [ "low", "high" ],
                                "Default": 0,
                                "Description": "q_test_sub quality group",
                                "Settings": {
                                    "a_cvar": [123,234]
                                }
                            }
                        }
                    }
                }
            }
            )",
            AZ::SettingsRegistryInterface::Format::JsonMergePatch,
            "");

        m_qualitySystemComponent->Activate();

        int value = -1;
        EXPECT_EQ(m_console->GetCvarValue("q_test", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, 1);

        EXPECT_EQ(m_console->GetCvarValue("q_test_sub", value), AZ::GetValueResult::Success);
        EXPECT_EQ(value, 0);
    }

    AZ_CVAR(int32_t, a_setting, 0, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example integer setting");
    AZ_CVAR(AZ::CVarFixedString, b_setting, "default", nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "Example string setting");

    TEST_F(QualitySystemComponentTestFixture, QualitySystem_Loads_Group_Level)
    {
        m_settingsRegistry->MergeSettings(R"(
            {
                "O3DE": {
                    "Quality": {
                        "Groups": {
                            "q_test": {
                                "Levels": [ "low", "medium","high", "veryhigh"],
                                "Default": 2,
                                "Description": "q_test quality group",
                                "Settings": {
                                    "a_setting": [0,1,2,3],
                                    "b_setting": ["a","b","c","d"]
                                }
                            },
                        }
                    }
                }
            }
            )",
            AZ::SettingsRegistryInterface::Format::JsonMergePatch,
            "");
        m_qualitySystemComponent->Activate();

        int intValue = -1;
        AZ::CVarFixedString stringValue;

        // when the cvar values are queried before the group level is loaded 
        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);

        // expect the value is default
        EXPECT_EQ(intValue, 0);
        EXPECT_STREQ(stringValue.c_str(), "default");

        // when the group level is loaded
        QualitySystemEvents::Bus::Broadcast(&QualitySystemEvents::LoadGroupQualityLevel, "q_test", 1);

        // expect the values are set based on the group level settings
        EXPECT_EQ(m_console->GetCvarValue("a_setting", intValue), AZ::GetValueResult::Success);
        EXPECT_EQ(intValue, 1);

        EXPECT_EQ(m_console->GetCvarValue("b_setting", stringValue), AZ::GetValueResult::Success);
        EXPECT_STREQ(stringValue.c_str(), "b");
    }

    TEST_F(QualitySystemComponentTestFixture, QualitySystem_Gets_Group_Level)
    {
        m_settingsRegistry->MergeSettings(R"(
            {
                "O3DE": {
                    "Quality": {
                        "Groups": {
                            "q_test": {
                                "Levels": [ "low", "medium","high","veryhigh" ],
                                "Default": 2,
                            },
                        }
                    }
                }
            }
            )",
            AZ::SettingsRegistryInterface::Format::JsonMergePatch,
            "");
        m_qualitySystemComponent->Activate();

        // when the group level is queried
        int value = -1;
        QualitySystemEvents::Bus::BroadcastResult(value, &QualitySystemEvents::GetGroupQualityLevel, "q_test");

        // expect the default level is returned
        EXPECT_EQ(value, 2);
    }
} // namespace UnitTest

