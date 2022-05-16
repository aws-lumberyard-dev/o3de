/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProfilerScriptBindings.h>

#include <ScriptAutomationSystemComponent.h>
#include <ScriptAutomation/ScriptAutomationBus.h>

#include <AzCore/Debug/Budget.h>
#include <AzCore/Debug/IEventLogger.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>
#include <AzCore/std/string/fixed_string.h>

namespace ScriptAutomation
{
    namespace Bindings
    {
        void CaptureStatistics(AZ::ScriptDataContext& dc)
        {
            if (dc.GetNumArguments() != 2)
            {
                AZ_Assert(false, "CaptureStatistics() expects two parameters");
                return;
            }

            if (!dc.IsNumber(0))
            {
                AZ_Assert(false, "CaptureStatistics() expects an interger as its first parameter");
                return;
            }

            if (!dc.IsTable(1))
            {
                AZ_Assert(false, "CaptureStatistics() expects a table as its second parameter");
                return;
            }

            int numFrames = 0;
            if (!dc.ReadArg(0, numFrames))
            {
                AZ_Assert(false, "Failed to read number of frames to capture");
                return;
            }

            auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
            auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

            const char* moduleName;
            int moduleIndex;
            int unused;
            dc.InspectTable(1, dc);
            while (dc.InspectNextElement(moduleIndex, moduleName, unused))
            {
                AZ::Crc32 moduleCrc = AZ::Crc32(moduleName);
                auto& profiler = AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get()->GetProfiler(moduleCrc);
                automationComponent->AddProfiler(moduleCrc);

                AZ::ScriptDataContext moduleEvents;
                if (!dc.InspectTable(moduleIndex, moduleEvents))
                {
                    AZ_Assert(false, "Event names should be specified as an array");
                    return;
                }

                int eventIndex;
                const char* fieldName;
                while (moduleEvents.InspectNextElement(eventIndex, fieldName, unused))
                {
                    AZStd::string eventName;
                    moduleEvents.ReadValue(eventIndex, eventName);

                    AZ::Crc32 eventCrc = AZ::Crc32(eventName.c_str());
                    profiler.GetStatsManager().AddStatistic(eventCrc, eventName, "us");
                }
            }

            auto enableProfilers = [numFrames]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                automationComponent->ActivateAllProfilers();
                ScriptAutomationInterface->SetIdleFrames(numFrames);
            };

            auto disableFrofilers = []()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                automationComponent->DeactivateAllProfilers();
            };

            ScriptAutomationInterface->QueueScriptOperation(AZStd::move(enableProfilers));
            ScriptAutomationInterface->QueueScriptOperation(AZStd::move(disableFrofilers));
        }

        void ToggleProfileCapture(int numFrames)
        {
            auto stopEventLogging = []()
            {
                if (auto logger = AZ::Interface<AZ::Debug::IEventLogger>::Get(); logger)
                {
                    logger->Stop();
                }
            };

            auto startPerformanceLogging = [numFrames]()
            {
                if (auto registry = AZ::SettingsRegistry::Get(); registry)
                {
                    AZ::IO::FixedMaxPath outputPath;
                    registry->Get(outputPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_DevWriteStorage);
                    outputPath /= "perf_metrics";
                    AZ::IO::FixedMaxPathString baseFileName{ "PerfMetrics" };

                    if (auto logger = AZ::Interface<AZ::Debug::IEventLogger>::Get(); logger)
                    {
                        logger->Start(outputPath.Native(), baseFileName, true);
                        auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                        ScriptAutomationInterface->SetIdleFrames(numFrames);
                    }
                }
            };

            auto stopPerformanceLogging = []()
            {
                if (auto logger = AZ::Interface<AZ::Debug::IEventLogger>::Get(); logger)
                {
                    logger->Stop();
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(stopEventLogging);
            ScriptAutomationInterface::Get()->QueueScriptOperation(startPerformanceLogging);
            ScriptAutomationInterface::Get()->QueueScriptOperation(stopPerformanceLogging);
        }
    }

    void ReflectProfilerScriptBindings(AZ::BehaviorContext* context)
    {
        context->Method("CaptureStatistics", &Bindings::CaptureStatistics);
        context->Method("ToggleProfileCapture", &Bindings::ToggleProfileCapture);
    }
}
