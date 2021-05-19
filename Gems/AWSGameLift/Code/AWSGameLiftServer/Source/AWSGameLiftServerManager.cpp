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
#include <GameLiftServerSDKWrapper.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AWSGameLift
{
    AWSGameLiftServerManager::AWSGameLiftServerManager(const GameLiftServerProcessDesc& desc)
        : m_serverProcessDesc(desc)
        , m_serverProcessInitOutcome(nullptr)
        , m_serverSDKInitialized(false)
    {
    }

    AWSGameLiftServerManager::~AWSGameLiftServerManager()
    {
        m_gameLiftServerSDKWrapper.reset();
    }

    AZStd::weak_ptr<GameLiftServerSDKWrapper> AWSGameLiftServerManager::GetGameLiftServerSDKWrapper()
    {
        if (!m_gameLiftServerSDKWrapper)
        {
            m_gameLiftServerSDKWrapper = AZStd::make_shared<GameLiftServerSDKWrapper>();
        }

        return m_gameLiftServerSDKWrapper;
    }

    bool AWSGameLiftServerManager::InitializeGameLiftServerSDK()
    {
        if (m_serverSDKInitialized)
        {
            AZ_Error("AWSGameLift", false, "Server process has already been initialized.\n");
            return false;
        }

        Aws::GameLift::Server::InitSDKOutcome initOutcome = GetGameLiftServerSDKWrapper().lock()->InitSDK();
        m_serverSDKInitialized = initOutcome.IsSuccess();

        AZ_Error("AWSGameLift", m_serverSDKInitialized, "Failed to initialize GameLift Server SDK.\n");

        return m_serverSDKInitialized;
    }

    bool AWSGameLiftServerManager::NotifyGameLiftProcessReady()
    {
        if (!m_serverSDKInitialized)
        {
            AZ_Error("AWSGameLift", false, "GameLift Server SDK has not been initialized.\n");
            return false;
        }

        AZ_Warning("AWSGameLift", m_serverProcessDesc.m_port != 0, "Server will be listening on ephemeral port");

        std::vector<std::string> logPaths;
        for (const std::string& path : m_serverProcessDesc.m_logPaths)
        {
            logPaths.push_back(path.c_str());
        }

        Aws::GameLift::Server::ProcessParameters processReadyParameter = Aws::GameLift::Server::ProcessParameters(
            AZStd::bind(&AWSGameLiftServerManager::OnStartGameSession, this, AZStd::placeholders::_1),
            AZStd::bind(&AWSGameLiftServerManager::OnUpdateGameSession, this),
            AZStd::bind(&AWSGameLiftServerManager::OnProcessTerminate, this), AZStd::bind(&AWSGameLiftServerManager::OnHealthCheck, this),
            m_serverProcessDesc.m_port, Aws::GameLift::Server::LogParameters(logPaths));

        m_serverProcessInitOutcome =
            new Aws::GameLift::GenericOutcomeCallable(GetGameLiftServerSDKWrapper().lock()->ProcessReadyAsync(processReadyParameter));

        return true;
    }

    bool AWSGameLiftServerManager::ShutDownGameSession()
    {
        return OnProcessTerminate();
    }

    void AWSGameLiftServerManager::OnStartGameSession(Aws::GameLift::Server::Model::GameSession)
    {
        // TODO: Game-specific tasks when starting a new game session, such as loading map.
    }

    bool AWSGameLiftServerManager::OnProcessTerminate()
    {
        if (!m_serverSDKInitialized)
        {
            AZ_Error("AWSGameLift", false, "No server process was initialized.\n");
            return false;
        }

        // TODO: Game-specific tasks required to gracefully shut down the game session and the server process.

        Aws::GameLift::GenericOutcome processEndingOutcome = GetGameLiftServerSDKWrapper().lock()->ProcessEnding();
        bool processEndingIsSuccess = processEndingOutcome.IsSuccess();

        AZ_Error(
            "AWSGameLift", processEndingIsSuccess, "Attempt to end process failed:%s:%s\n",
            processEndingOutcome.GetError().GetErrorName().c_str(), processEndingOutcome.GetError().GetErrorMessage().c_str());

        return processEndingIsSuccess;
    }

    bool AWSGameLiftServerManager::OnHealthCheck()
    {
        // TODO: Complete health evaluation within 60 seconds and set health
        return true;
    }

    void AWSGameLiftServerManager::OnUpdateGameSession()
    {
        // TODO: Perform game-specific tasks to prep for newly matched players
        return;
    }
} // namespace AWSGameLift
