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

#include <AWSGameLiftServerManager.h>
#include <AWSGameLiftSessionConstants.h>
#include <GameLiftServerSDKWrapper.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/std/bind/bind.h>
#include <AzFramework/Session/SessionNotifications.h>

namespace AWSGameLift
{
    AWSGameLiftServerManager::AWSGameLiftServerManager()
        : m_serverSDKInitialized(false)
        , m_gameLiftServerSDKWrapper(AZStd::make_unique<GameLiftServerSDKWrapper>())
    {
    }

    AWSGameLiftServerManager::~AWSGameLiftServerManager()
    {
        m_gameLiftServerSDKWrapper.reset();
    }

    AzFramework::SessionConfig AWSGameLiftServerManager::BuildSessionConfig(const Aws::GameLift::Server::Model::GameSession& gameSession)
    {
        AzFramework::SessionConfig sessionConfig;

        sessionConfig.m_dnsName = gameSession.GetDnsName().c_str();
        AZStd::string propertiesOutput = "";
        for (const auto& gameProperty : gameSession.GetGameProperties())
        {
            sessionConfig.m_sessionProperties.emplace(gameProperty.GetKey().c_str(), gameProperty.GetValue().c_str());
            propertiesOutput += AZStd::string::format("{Key=%s,Value=%s},", gameProperty.GetKey().c_str(), gameProperty.GetValue().c_str());
        }
        if (!propertiesOutput.empty())
        {
            propertiesOutput = propertiesOutput.substr(0, propertiesOutput.size() - 1); // Trim last comma to fit array format
        }
        sessionConfig.m_sessionId = gameSession.GetGameSessionId().c_str();
        sessionConfig.m_ipAddress = gameSession.GetIpAddress().c_str();
        sessionConfig.m_maxPlayer = gameSession.GetMaximumPlayerSessionCount();
        sessionConfig.m_sessionName = gameSession.GetName().c_str();
        sessionConfig.m_port = gameSession.GetPort();
        sessionConfig.m_status = AWSGameLiftSessionStatusNames[(int)gameSession.GetStatus()];

        AZ_TracePrintf(AWSGameLiftServerManagerName,
            "Built SessionConfig with Name=%s, Id=%s, Status=%s, DnsName=%s, IpAddress=%s, Port=%d, MaxPlayer=%d and Properties=%s",
            sessionConfig.m_sessionName.c_str(),
            sessionConfig.m_sessionId.c_str(),
            sessionConfig.m_status.c_str(),
            sessionConfig.m_dnsName.c_str(),
            sessionConfig.m_ipAddress.c_str(),
            sessionConfig.m_port,
            sessionConfig.m_maxPlayer,
            AZStd::string::format("[%s]", propertiesOutput.c_str()).c_str());

        return sessionConfig;
    }

    AZStd::string AWSGameLiftServerManager::GetSessionCertificate()
    {
        return "";
    }

    bool AWSGameLiftServerManager::InitializeGameLiftServerSDK()
    {
        if (m_serverSDKInitialized)
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerSDKAlreadyInitErrorMessage);
            return false;
        }

        AZ_TracePrintf(AWSGameLiftServerManagerName, "Initiating Amazon GameLift Server SDK...");
        Aws::GameLift::Server::InitSDKOutcome initOutcome = m_gameLiftServerSDKWrapper->InitSDK();
        m_serverSDKInitialized = initOutcome.IsSuccess();

        AZ_Error(AWSGameLiftServerManagerName, m_serverSDKInitialized,
            AWSGameLiftServerInitSDKErrorMessage, initOutcome.GetError().GetErrorMessage().c_str());

        return m_serverSDKInitialized;
    }

    void AWSGameLiftServerManager::HandleDestroySession()
    {
        OnProcessTerminate();
    }

    void AWSGameLiftServerManager::HandlePlayerLeaveSession(const AzFramework::PlayerConnectionConfig& playerConnectionConfig)
    {
        // TODO: Perform player data cleanup in game session after player has disconnected from server
        AZ_UNUSED(playerConnectionConfig);
    }

    bool AWSGameLiftServerManager::NotifyGameLiftProcessReady(const GameLiftServerProcessDesc& desc)
    {
        if (!m_serverSDKInitialized)
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerSDKNotInitErrorMessage);
            return false;
        }

        AZ_Warning(AWSGameLiftServerManagerName, desc.m_port != 0, AWSGameLiftServerTempPortErrorMessage);

        AZ::JobContext* jobContext = nullptr;
        AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);
        AZ::Job* processReadyJob = AZ::CreateJobFunction(
            [this, desc]() {
                // The GameLift ProcessParameters object expects an vector (std::vector) of standard strings (std::string) as the log paths.
                std::vector<std::string> logPaths;
                for (const AZStd::string& path : desc.m_logPaths)
                {
                    logPaths.push_back(path.c_str());
                }

                Aws::GameLift::Server::ProcessParameters processReadyParameter = Aws::GameLift::Server::ProcessParameters(
                    AZStd::bind(&AWSGameLiftServerManager::OnStartGameSession, this, AZStd::placeholders::_1),
                    AZStd::bind(&AWSGameLiftServerManager::OnUpdateGameSession, this),
                    AZStd::bind(&AWSGameLiftServerManager::OnProcessTerminate, this),
                    AZStd::bind(&AWSGameLiftServerManager::OnHealthCheck, this), desc.m_port,
                    Aws::GameLift::Server::LogParameters(logPaths));

                AZ_TracePrintf(AWSGameLiftServerManagerName, "Notifying GameLift server process is ready...");
                auto processReadyOutcome = m_gameLiftServerSDKWrapper->ProcessReady(processReadyParameter);

                if (!processReadyOutcome.IsSuccess())
                {
                    AZ_Error(AWSGameLiftServerManagerName, false,
                        AWSGameLiftServerProcessReadyErrorMessage, processReadyOutcome.GetError().GetErrorMessage().c_str());
                    this->HandleDestroySession();
                }
        }, true, jobContext);
        processReadyJob->Start();
        return true;
    }

    void AWSGameLiftServerManager::OnStartGameSession(const Aws::GameLift::Server::Model::GameSession& gameSession)
    {
        AzFramework::SessionConfig sessionConfig = BuildSessionConfig(gameSession);

        bool createSessionResult = true;
        AZ::EBusReduceResult<bool&, AZStd::logical_and<bool>> result(createSessionResult);
        AzFramework::SessionNotificationBus::BroadcastResult(
            result, &AzFramework::SessionNotifications::OnCreateSessionBegin, sessionConfig);

        if (createSessionResult)
        {
            AZ_TracePrintf(AWSGameLiftServerManagerName, "Activating GameLift game session...");
            Aws::GameLift::GenericOutcome activationOutcome = m_gameLiftServerSDKWrapper->ActivateGameSession();

            if (activationOutcome.IsSuccess())
            {
                // Register server manager as handler once game session has been activated
                if (!AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Get())
                {
                    AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Register(this);
                }
            }
            else
            {
                AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerActivateGameSessionErrorMessage,
                    activationOutcome.GetError().GetErrorMessage().c_str());
                HandleDestroySession();
            }
        }
        else
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerGameInitErrorMessage);
            HandleDestroySession();
        }
    }

    bool AWSGameLiftServerManager::OnProcessTerminate()
    {
        // Send notifications to handler(s) to gracefully shut down the server process.
        bool destroySessionResult = true;
        AZ::EBusReduceResult<bool&, AZStd::logical_and<bool>> result(destroySessionResult);
        AzFramework::SessionNotificationBus::BroadcastResult(result, &AzFramework::SessionNotifications::OnDestroySessionBegin);

        if (destroySessionResult)
        {
            // No further request should be handled by GameLift server manager at this point
            if (AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Get())
            {
                AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Unregister(this);
            }
        }
        else
        {
            AZ_Error("AWSGameLift", false, AWSGameLiftServerGameSessionDestroyErrorMessage);
            return false;
        }

        // Notifies the GameLift service that the server process is shutting down.
        if (!m_serverSDKInitialized)
        {
            AZ_Error(AWSGameLiftServerManagerName, false, AWSGameLiftServerSDKNotInitErrorMessage);
            return false;
        }

        // TODO: Game-specific tasks required to gracefully shut down the game session and the server process.

        AZ_TracePrintf(AWSGameLiftServerManagerName, "Notifying GameLift server process is ending...");
        Aws::GameLift::GenericOutcome processEndingOutcome = m_gameLiftServerSDKWrapper->ProcessEnding();
        bool processEndingIsSuccess = processEndingOutcome.IsSuccess();

        AZ_Error(AWSGameLiftServerManagerName, processEndingIsSuccess,
            AWSGameLiftServerProcessEndingErrorMessage, processEndingOutcome.GetError().GetErrorMessage().c_str());

        return processEndingIsSuccess;
    }

    bool AWSGameLiftServerManager::OnHealthCheck()
    {
        bool healthCheckResult = true;
        AZ::EBusReduceResult<bool&, AZStd::logical_and<bool>> result(healthCheckResult);
        AzFramework::SessionNotificationBus::BroadcastResult(result, &AzFramework::SessionNotifications::OnSessionHealthCheck);

        return m_serverSDKInitialized && healthCheckResult;
    }

    void AWSGameLiftServerManager::OnUpdateGameSession()
    {
        // TODO: Perform game-specific tasks to prep for newly matched players
        return;
    }

    void AWSGameLiftServerManager::SetGameLiftServerSDKWrapper(AZStd::unique_ptr<GameLiftServerSDKWrapper> gameLiftServerSDKWrapper)
    {
        m_gameLiftServerSDKWrapper.reset();
        m_gameLiftServerSDKWrapper = AZStd::move(gameLiftServerSDKWrapper);
    }

    bool AWSGameLiftServerManager::ValidatePlayerJoinSession(const AzFramework::PlayerConnectionConfig& playerConnectionConfig)
    {
        // TODO: Perform connection validation for new joined player on server side
        AZ_UNUSED(playerConnectionConfig);
        return false;
    }
} // namespace AWSGameLift
