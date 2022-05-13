/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomationSystemComponent.h>

#include <AssetStatusTracker.h>
#include <ScriptAutomationScriptBindings.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Statistics/StatisticalProfilerProxy.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Script/ScriptComponent.h>

namespace ScriptAutomation
{
    namespace
    {
        AZ::Data::Asset<AZ::ScriptAsset> LoadRawLuaScript(const AZStd::string& productPath, const AZ::IO::FixedMaxPath& resolvedPath, AZ::ScriptContext& context)
        {
            AZ::IO::FileIOStream inputStream;
            if (inputStream.Open(resolvedPath.c_str(), AZ::IO::OpenMode::ModeRead))
            {
                AzFramework::ScriptCompileRequest compileRequest;
                compileRequest.m_sourceFile = resolvedPath.c_str();
                compileRequest.m_input = &inputStream;

                auto outcome = AzFramework::CompileScript(compileRequest, context);
                inputStream.Close();
                if (outcome.IsSuccess())
                {
                    AZ::Uuid id = AZ::Uuid::CreateName(productPath.c_str());
                    AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = AZ::Data::AssetManager::Instance().FindOrCreateAsset<AZ::ScriptAsset>(AZ::Data::AssetId(id)
                        , AZ::Data::AssetLoadBehavior::Default);
                    scriptAsset.Get()->m_data = compileRequest.m_luaScriptDataOut;

                    return scriptAsset;
                }
                else
                {
                    AZ_Assert(false, "Failed to compile script asset '%s'. Reason: '%s'", resolvedPath.c_str(), outcome.GetError().c_str());
                    return {};
                }
            }
            
            AZ_Assert(false, "Unable to find product asset '%s'. Has the source asset finished building?", resolvedPath.c_str());
            return {};
        }

        AZ::Data::Asset<AZ::ScriptAsset> LoadCompiledLuaScript(const AZStd::string& productPath)
        {
            AZ::Data::AssetId assetId;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                assetId,
                &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                productPath.c_str(),
                AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid(),
                true);

            if (assetId.IsValid())
            {
                AZ::Data::Asset<AZ::ScriptAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::ScriptAsset>(
                    assetId, AZ::Data::AssetLoadBehavior::PreLoad);
                asset.BlockUntilLoadComplete();

                if (!asset.IsReady())
                {
                    AZ_Assert(false, "Could not load '%s'", productPath.c_str());
                    return {};
                }

                return asset;
            }

            AZ_Assert(false, "Unable to find product asset '%s'. Has the source asset finished building?", productPath.c_str());
            return {};
        }

        AZ::Data::Asset<AZ::ScriptAsset> LoadScriptAssetFromPath(const AZStd::string& productPath, AZ::ScriptContext& context)
        {
            AZ::IO::FixedMaxPath resolvedPath;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedPath, productPath.c_str());

            if (resolvedPath.Extension() == ".lua")
            {
                return LoadRawLuaScript(productPath, resolvedPath, context);
            }
            else if (resolvedPath.Extension() == ".luac")
            {
                return LoadCompiledLuaScript(productPath);
            }

            AZ_Assert(false, "'%s' is not a script asset!", resolvedPath.c_str());
            return {};
        }
    } // namespace

    void ScriptAutomationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptAutomationSystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ScriptAutomationSystemComponent>("ScriptAutomation", "Provides a mechanism for automating various tasks through Lua scripting in the game launchers")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void ScriptAutomationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AutomationServiceCrc);
    }

    void ScriptAutomationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AutomationServiceCrc);
    }

    void ScriptAutomationSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ScriptAutomationSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ScriptAutomationSystemComponent::ScriptAutomationSystemComponent()
    {
        if (ScriptAutomationInterface::Get() == nullptr)
        {
            ScriptAutomationInterface::Register(this);
        }
    }

    ScriptAutomationSystemComponent::~ScriptAutomationSystemComponent()
    {
        if (ScriptAutomationInterface::Get() == this)
        {
            ScriptAutomationInterface::Unregister(this);
        }
    }

    void ScriptAutomationSystemComponent::SetIdleFrames(int numFrames)
    {
        AZ_Assert(m_scriptIdleFrames == 0, "m_scriptIdleFrames is being stomped");
        m_scriptIdleFrames = numFrames;
    }

    void ScriptAutomationSystemComponent::SetIdleSeconds(float numSeconds)
    {
        AZ_Assert(m_scriptIdleSeconds <= 0, "m_scriptIdleSeconds is being stomped");
        m_scriptIdleSeconds = numSeconds;
    }

    void ScriptAutomationSystemComponent::SetFrameTimeIsLocked(bool frameTimeIsLocked)
    {
        m_frameTimeIsLocked = frameTimeIsLocked;
    }

    int ScriptAutomationSystemComponent::GetRandomTestSeed()
    {
        return m_randomSeed;
    }

    void ScriptAutomationSystemComponent::StartTrackingAssetStatus()
    {
        m_assetStatusTracker.StartTracking();
    }

    void ScriptAutomationSystemComponent::StopTrackingAssetStatus()
    {
        m_assetStatusTracker.StopTracking();
    }

    void ScriptAutomationSystemComponent::ExpectAsset(const AZStd::string& sourceAssetPath, uint32_t expectedCount)
    {
        m_assetStatusTracker.ExpectAsset(sourceAssetPath, expectedCount);
    }

    void ScriptAutomationSystemComponent::IdleUntilExpectedAssetsFinish(float timeout)
    {
        AZ_Assert(!m_waitForAssetTracker, "It shouldn't be possible to run the next command until m_waitForAssetTracker is false");

        m_waitForAssetTracker = true;
        m_assetTrackingTimeout = timeout;
    }

    void ScriptAutomationSystemComponent::AddProfiler(AZ::Crc32 profilerId)
    {
        m_profilerTracker.push_back(profilerId);
    }

    void ScriptAutomationSystemComponent::ActivateAllProfilers()
    {
        for (auto& profilerId : m_profilerTracker)
        {
            AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get()->ActivateProfiler(profilerId, true, false);
        }
    }

    void ScriptAutomationSystemComponent::DeactivateAllProfilers()
    {
        for (auto& profilerId : m_profilerTracker)
        {
            AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get()->ActivateProfiler(profilerId, false, false);
            AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get()->GetProfiler(profilerId).LogAndResetStats("ScriptAutomation");
            AZ::Interface<AZ::Statistics::StatisticalProfilerProxy>::Get()->GetProfiler(profilerId).GetStatsManager().Clear();
        }

        m_profilerTracker.clear();
    }

    void ScriptAutomationSystemComponent::Activate()
    {
        ScriptAutomationRequestBus::Handler::BusConnect();

        m_scriptContext = AZStd::make_unique<AZ::ScriptContext>();
        m_scriptBehaviorContext = AZStd::make_unique<AZ::BehaviorContext>();

        ReflectScriptBindings(m_scriptBehaviorContext.get());
        m_scriptContext->BindTo(m_scriptBehaviorContext.get());

        AZ::ComponentApplication* application = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(application, &AZ::ComponentApplicationBus::Events::GetApplication);
        if (application)
        {
            constexpr const char* automationSuiteSwitch = "run-automation-suite";
            constexpr const char* automationExitSwitch = "exit-on-automation-end";
            constexpr const char* automationRandomSeedSwitch = "random-test-seed";

            auto commandLine = application->GetAzCommandLine();
            if (commandLine->HasSwitch(automationSuiteSwitch))
            {
                m_isStarted = false;
                m_automationScript = commandLine->GetSwitchValue(automationSuiteSwitch, 0);
                m_exitOnFinish = commandLine->HasSwitch(automationExitSwitch);

                if (commandLine->HasSwitch(automationRandomSeedSwitch))
                {
                    m_randomSeed = atoi(commandLine->GetSwitchValue(automationRandomSeedSwitch, 0).c_str());
                }

                AZ::TickBus::Handler::BusConnect();
            }
        }
    }

    void ScriptAutomationSystemComponent::Deactivate()
    {
        m_scriptContext = nullptr;
        m_scriptBehaviorContext = nullptr;

        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }

        ScriptAutomationRequestBus::Handler::BusDisconnect();
    }

    void ScriptAutomationSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_isStarted)
        {
            ScriptAutomationEventsBus::Broadcast(&ScriptAutomationEventsBus::Events::CustomReflect, m_scriptBehaviorContext.get());
            
            m_isStarted = true;
            SetupScriptExecution(m_automationScript);
            ExecuteScript(m_automationScript);
            return;
        }

        if (m_executingScripts.size() > 0 || !m_scriptOperations.empty())
        {
            ScriptAutomationEventsBus::Broadcast(&ScriptAutomationEventsBus::Events::PreTick);

            while (!m_scriptOperations.empty())
            {
                if (m_scriptPaused)
                {
                    m_scriptPauseTimeout -= deltaTime;
                    if (m_scriptPauseTimeout < 0)
                    {
                        AZ_Error("ScriptAutomation", false, "Script pause timed out. Continuing...");
                        m_scriptPaused = false;
                    }
                    else
                    {
                        break;
                    }
                }

                if (m_waitForAssetTracker)
                {
                    m_assetTrackingTimeout -= deltaTime;
                    if (m_assetTrackingTimeout < 0)
                    {
                        auto incomplateAssetList = m_assetStatusTracker.GetIncompleteAssetList();
                        AZStd::string incompleteAssetListString;
                        AzFramework::StringFunc::Join(incompleteAssetListString, incomplateAssetList.begin(), incomplateAssetList.end(), "\n    ");
                        AZ_Error("ScriptAutomation", false, "Script asset tracking timed out waiting for:\n    %s \n Continuing...", incompleteAssetListString.c_str());
                        m_waitForAssetTracker = false;
                    }
                    else if (m_assetStatusTracker.DidExpectedAssetsFinish())
                    {
                        AZ_Printf("ScriptAutomation", "Asset Tracker finished with %f seconds remaining.", m_assetTrackingTimeout);
                        m_waitForAssetTracker = false;
                    }
                    else
                    {
                        break;
                    }
                }

                if (m_scriptIdleFrames > 0)
                {
                    m_scriptIdleFrames--;
                    break;
                }

                if (m_scriptIdleSeconds > 0)
                {
                    m_scriptIdleSeconds -= deltaTime;
                    break;
                }

                // Execute the next operation
                m_scriptOperations.front()();

                m_scriptOperations.pop();
            }

            ScriptAutomationEventsBus::Broadcast(&ScriptAutomationEventsBus::Events::PostTick);
        }
        else
        {
            AZ_Assert(m_scriptPaused == false, "Script manager is in an unexpected state.");
            AZ_Assert(m_scriptIdleFrames == 0, "Script manager is in an unexpected state.");
            AZ_Assert(m_scriptIdleSeconds <= 0.0f, "Script manager is in an unexpected state.");
            AZ_Assert(m_waitForAssetTracker == false, "Script manager is in an unexpected state.");
            AZ_Assert(m_executingScripts.size() == 0, "Script manager is in an unexpected state");

            m_assetStatusTracker.StopTracking();

            if (m_frameTimeIsLocked)
            {
                AZ::Interface<AZ::IConsole>::Get()->PerformCommand("t_simulationTickDeltaOverride 0");
                m_frameTimeIsLocked = false;
            }

            if (m_shouldRestoreViewportSize)
            {
                Utils::ResizeClientArea(m_savedViewportWidth, m_savedViewportHeight);
                m_shouldRestoreViewportSize = false;
            }

            ScriptAutomationEventsBus::Broadcast(&ScriptAutomationEventsBus::Events::AutomationFinished);

            if (m_exitOnFinish)
            {
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::ExitMainLoop);
            }
        }
    }

    AZ::BehaviorContext* ScriptAutomationSystemComponent::GetAutomationContext()
    {
        return m_scriptBehaviorContext.get();
    }

    void ScriptAutomationSystemComponent::PauseAutomation(float timeout)
    {
        m_scriptPaused = true;
        m_scriptPauseTimeout = AZ::GetMax(timeout, m_scriptPauseTimeout);
    }

    void ScriptAutomationSystemComponent::ResumeAutomation()
    {
        AZ_Warning("ScriptAutomation", m_scriptPaused, "Script is not paused");
        m_scriptPaused = false;
    }

    void ScriptAutomationSystemComponent::Abort(const AZStd::string& reason)
    {
        AZ_TracePrintf("ScriptAutomation", "Script execution aborted. Reason: '%s'", reason.c_str());

        m_scriptOperations = {};
        m_executingScripts.clear();
        m_scriptPaused = false;
        m_scriptIdleFrames = 0;
        m_scriptIdleSeconds = 0.0f;
        m_waitForAssetTracker = false;
    }

    void ScriptAutomationSystemComponent::QueueScriptOperation(ScriptAutomationRequests::ScriptOperation&& operation)
    {
        m_scriptOperations.push(AZStd::move(operation));
    }

    void ScriptAutomationSystemComponent::ExecuteScript(const AZStd::string& scriptFilePath)
    {
        AZ::Data::Asset<AZ::ScriptAsset> scriptAsset = LoadScriptAssetFromPath(scriptFilePath, *m_scriptContext.get());
        if (!scriptAsset)
        {
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("ScriptAutomation", false, "Script: Could not find or load script asset '%s'.", scriptFilePath.c_str());
                }
            );
            return;
        }

        if (m_executingScripts.find(scriptAsset.GetId()) != m_executingScripts.end())
        {
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("ScriptAutomation", false, "Calling script '%s' would likely cause an infinite loop and crash. Skipping.", scriptFilePath.c_str());
                }
            );
            return;
        }

        QueueScriptOperation([scriptFilePath]()
            {
                AZ_Printf("ScriptAutomation", "Running script '%s'...\n", scriptFilePath.c_str());
            }
        );

        ScriptAutomationEventsBus::Broadcast(&ScriptAutomationEventsBus::Events::PreScriptExecution, scriptFilePath);
        m_executingScripts.insert(scriptAsset.GetId());

        if (!m_scriptContext->Execute(scriptAsset->m_data.GetScriptBuffer().data(), scriptFilePath.c_str(), scriptAsset->m_data.GetScriptBuffer().size()))
        {
            // Push an error operation on the back of the queue instead of reporting it immediately so it doesn't get lost
            // in front of a bunch of queued m_scriptOperations.
            QueueScriptOperation([scriptFilePath]()
                {
                    AZ_Error("ScriptAutomation", false, "Script: Error running script '%s'.", scriptFilePath.c_str());
                }
            );
        }

        m_executingScripts.erase(scriptAsset.GetId());
        ScriptAutomationEventsBus::Broadcast(&ScriptAutomationEventsBus::Events::PostScriptExecution, scriptFilePath);
    }

    void ScriptAutomationSystemComponent::SetupScriptExecution(const AZStd::string& scriptFilePath)
    {
        // Save the window size so we can restore it after running the script, in case the script calls ResizeViewport
        AzFramework::NativeWindowHandle defaultWindowHandle;
        AzFramework::WindowSize windowSize;
        AzFramework::WindowSystemRequestBus::BroadcastResult(defaultWindowHandle, &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);
        AzFramework::WindowRequestBus::EventResult(windowSize, defaultWindowHandle, &AzFramework::WindowRequests::GetClientAreaSize);
        m_savedViewportWidth = windowSize.m_width;
        m_savedViewportHeight = windowSize.m_height;
        if (m_savedViewportWidth > 0 && m_savedViewportHeight > 0)
        {
            m_shouldRestoreViewportSize = true;
        }

        AZ_Assert(m_executingScripts.empty(), "There should be no active scripts at this point");
        
        ScriptAutomationEventsBus::Broadcast(&ScriptAutomationEventsBus::Events::AutomationStarted, scriptFilePath);
    }
} // namespace ScriptAutomation
