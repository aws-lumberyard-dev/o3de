/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomationSystemComponent.h>

#include <ProfilerScriptBindings.h>
#include <ScriptAutomationScriptBindings.h>
#include <ScriptAutomationUtils.h>

#include <Atom/RPI.Public/RPISystemInterface.h>

#include <AzCore/Console/IConsole.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/Components/ConsoleBus.h>

namespace ScriptAutomation
{
    namespace Bindings
    {
        void Print(const AZStd::string& message)
        {
            auto func = [message]()
            {
                AZ_TracePrintf("ScriptAutomation", "Script: %s\n", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void Warning(const AZStd::string& message)
        {
            auto func = [message]()
            {
                AZ_Warning("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void Error(const AZStd::string& message)
        {
            auto func = [message]()
            {
                AZ_Error("ScriptAutomation", false, "Script: %s", message.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void ExecuteConsoleCommand(const AZStd::string& command)
        {
            auto func = [command]()
            {
                AzFramework::ConsoleRequestBus::Broadcast(
                    &AzFramework::ConsoleRequests::ExecuteConsoleCommand, command.c_str());
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void IdleFrames(int numFrames)
        {
            auto func = [numFrames]()
            {
                ScriptAutomationInterface::Get()->SetIdleFrames(numFrames);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void IdleSeconds(float numSeconds)
        {
            auto func = [numSeconds]()
            {
                ScriptAutomationInterface::Get()->SetIdleSeconds(numSeconds);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void LockFrameTime(float seconds)
        {
            auto func = [seconds]()
            {
                int milliseconds = static_cast<int>(seconds * 1000);
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand(AZStd::string::format("t_simulationTickDeltaOverride %d", milliseconds).c_str());

                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                automationComponent->SetFrameTimeIsLocked(true);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void UnlockFrameTime()
        {
            auto func = []()
            {
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand("t_simulationTickDeltaOverride 0");
                
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
                automationComponent->SetFrameTimeIsLocked(false);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void AssetTracking_Start()
        {
            auto func = []()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->StartTrackingAssetStatus();
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void AssetTracking_ExpectAsset(const AZStd::string& sourceAssetPath, uint32_t expectedCount)
        {
            auto func = [sourceAssetPath, expectedCount]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->ExpectAsset(sourceAssetPath, expectedCount);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void AssetTracking_IdleUntilExpectedAssetsFinish(float timeout)
        {
            auto func = [timeout]()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->IdleUntilExpectedAssetsFinish(timeout);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void AssetTracking_Stop()
        {
            auto func = []()
            {
                auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
                auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);

                automationComponent->StopTrackingAssetStatus();
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void ResizeViewport(uint32_t width, uint32_t height)
        {
            auto func = [width, height]()
            {
                if (Utils::SupportsResizeClientArea())
                {
                    Utils::ResizeClientArea(width, height);
                }
                else
                {
                    AZ_Error("ScriptAutomation", false, "ResizeViewport() is not supported on this platform");
                }
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        void LoadLevel(const AZStd::string& levelPath)
        {
            auto func = [levelPath]()
            {
                AZStd::fixed_vector<AZStd::string_view, 2> loadLevelCmd;
                loadLevelCmd.push_back("LoadLevel");
                loadLevelCmd.push_back(levelPath);
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand(loadLevelCmd);
            };

            ScriptAutomationInterface::Get()->QueueScriptOperation(AZStd::move(func));
        }

        float DegToRad(float degrees)
        {
            return AZ::DegToRad(degrees);
        }

        AZStd::string GetRenderApiName()
        {
            AZ::RPI::RPISystemInterface* rpiSystem = AZ::RPI::RPISystemInterface::Get();
            return rpiSystem->GetRenderApiName().GetCStr();
        }

        int GetRandomTestSeed()
        {
            auto ScriptAutomationInterface = ScriptAutomationInterface::Get();
            auto automationComponent = azrtti_cast<ScriptAutomationSystemComponent*>(ScriptAutomationInterface);
            return automationComponent->GetRandomTestSeed();
        }

        AZStd::string ResolvePath(const AZStd::string& path)
        {
            AZ::IO::FixedMaxPath resolvedPath;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, path.c_str());
            return resolvedPath.String();
        }

        AZStd::string NormalizePath(const AZStd::string& path)
        {
            AZStd::string normalizedPath = path;
            AZ::StringFunc::Path::Normalize(normalizedPath);
            return normalizedPath;
        }

        void RunScript(const AZStd::string& scriptFilePath)
        {
            // Unlike other Script_ callback functions, we process immediately instead of pushing onto the m_scriptOperations queue.
            // This function is special because running the script is what adds more commands onto the m_scriptOperations queue.
            ScriptAutomationInterface::Get()->ExecuteScript(scriptFilePath);
        }
    } // namespace Bindings

    void ReflectScriptBindings(AZ::BehaviorContext* behaviorContext)
    {
        AZ::MathReflect(behaviorContext);

        behaviorContext->Method("Print", &Bindings::Print);
        behaviorContext->Method("Warning", &Bindings::Warning);
        behaviorContext->Method("Error", &Bindings::Error);

        behaviorContext->Method("DegToRad", &Bindings::DegToRad);
        behaviorContext->Method("GetRenderApiName", &Bindings::GetRenderApiName);
        behaviorContext->Method("GetRandomTestSeed", &Bindings::GetRandomTestSeed);

        behaviorContext->Method("ExecuteConsoleCommand", &Bindings::ExecuteConsoleCommand);

        behaviorContext->Method("IdleFrames", &Bindings::IdleFrames);
        behaviorContext->Method("IdleSeconds", &Bindings::IdleSeconds);
        behaviorContext->Method("LockFrameTime", &Bindings::LockFrameTime);
        behaviorContext->Method("UnlockFrameTime", &Bindings::UnlockFrameTime);

        behaviorContext->Method("LoadLevel", &Bindings::LoadLevel);
        behaviorContext->Method("RunScript", &Bindings::RunScript);
        behaviorContext->Method("ResolvePath", &Bindings::ResolvePath);
        behaviorContext->Method("NormalizePath", &Bindings::NormalizePath);

        behaviorContext->Method("ResizeViewport", &Bindings::ResizeViewport);

        behaviorContext->Method("AssetTracking_Start", &Bindings::AssetTracking_Start);
        behaviorContext->Method("AssetTracking_Stop", &Bindings::AssetTracking_Stop);
        behaviorContext->Method("AssetTracking_IdleUntilExpectedAssetsFinish", &Bindings::AssetTracking_IdleUntilExpectedAssetsFinish);

        AZ::BehaviorParameterOverrides expectedCountDetails = {"expectedCount", "Expected number of asset jobs; default=1", aznew AZ::BehaviorDefaultValue(1u)};
        const AZStd::array<AZ::BehaviorParameterOverrides, 2> assetTrackingExpectAssetArgs = {{ AZ::BehaviorParameterOverrides{}, expectedCountDetails }};
        behaviorContext->Method("AssetTracking_ExpectAsset", &Bindings::AssetTracking_ExpectAsset, assetTrackingExpectAssetArgs);

        ReflectProfilerScriptBindings(behaviorContext);
    }
} // namespace ScriptAutomation
