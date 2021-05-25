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

#include <aws/gamelift/server/GameLiftServerAPI.h>
#include <aws/gamelift/server/model/GameSession.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>
#include <AzFramework/Session/SessionConfig.h>

namespace AWSGameLift
{
    class GameLiftServerSDKWrapper;

    //! GameLift server process settings.
    struct GameLiftServerProcessDesc
    {
        AZStd::vector<AZStd::string> m_logPaths; //!< Log paths the servers will write to. Both relative to the game root folder and absolute paths supported.

        uint16_t m_port = 0; //!< The port the server will be listening on.
    };

    //! Manage the server process for hosting game sessions via GameLiftServerSDK.
    class AWSGameLiftServerManager
        : public AzFramework::ISessionHandlingServerRequests
    {
    public:
        static constexpr const char AWSGameLiftServerManagerName[] = "AWSGameLiftServerManager";
        static constexpr const char AWSGameLiftServerSDKNotInitErrorMessage[] =
            "Amazon GameLift Server SDK is not initialized yet.";
        static constexpr const char AWSGameLiftServerSDKAlreadyInitErrorMessage[] =
            "Amazon GameLift Server SDK has already been initialized.";
        static constexpr const char AWSGameLiftServerTempPortErrorMessage[] =
            "No server port specified, server will be listening on ephemeral port.";
        static constexpr const char AWSGameLiftServerGameInitErrorMessage[] =
            "Failed to process game dependent initialization during OnStartGameSession.";

        static constexpr const char AWSGameLiftServerInitSDKErrorMessage[] =
            "Failed to initialize Amazon GameLift Server SDK. ErrorMessage: %s";
        static constexpr const char AWSGameLiftServerProcessReadyErrorMessage[] =
            "Failed to notify GameLift server process ready. ErrorMessage: %s";
        static constexpr const char AWSGameLiftServerActivateGameSessionErrorMessage[] =
            "Failed to activate GameLift game session. ErrorMessage: %s";
        static constexpr const char AWSGameLiftServerProcessEndingErrorMessage[] =
            "Failed to end notify GameLift server process ending. ErrorMessage: %s";

        AWSGameLiftServerManager();
        virtual ~AWSGameLiftServerManager();

        //! Initialize GameLift API client by calling InitSDK().
        //! @return Whether the initialization is successful.
        bool InitializeGameLiftServerSDK();

        //! Notify GameLift that the server process is ready to host a game session.
        //! @param desc GameLift server process settings.
        //! @return Whether the ProcessReady notification is sent to GameLift.
        bool NotifyGameLiftProcessReady(const GameLiftServerProcessDesc& desc);

        // ISessionHandlingServerRequests interface implementation
        void HandleDestroySession() override;
        bool ValidatePlayerJoinSession(const AzFramework::PlayerConnectionConfig& playerConnectionConfig) override;
        void HandlePlayerLeaveSession(const AzFramework::PlayerConnectionConfig& playerConnectionConfig) override;

    protected:
        void SetGameLiftServerSDKWrapper(AZStd::unique_ptr<GameLiftServerSDKWrapper> gameLiftServerSDKWrapper);

    private:
        // Build session config by using AWS GameLift Server GameSession Model
        AzFramework::SessionConfig BuildSessionConfig(const Aws::GameLift::Server::Model::GameSession& gameSession);

        //! Callback function that the GameLift service invokes to activate a new game session.
        void OnStartGameSession(const Aws::GameLift::Server::Model::GameSession& gameSession);

        //! Callback function that the GameLift service invokes to pass an updated game session object to the server process.
        void OnUpdateGameSession();

        //! Callback function that the server process or GameLift service invokes to force the server process to shut down.
        //! @return whether the server process is terminated and the ProcessEnding notification is sent to GameLift.
        bool OnProcessTerminate();

        //! Callback function that the GameLift service invokes to request a health status report from the server process.
        //! @return Whether the server process is healthy.
        bool OnHealthCheck();

        AZStd::unique_ptr<GameLiftServerSDKWrapper> m_gameLiftServerSDKWrapper;
        bool m_serverSDKInitialized;
    };
} // namespace AWSGameLift
