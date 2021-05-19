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

namespace AWSGameLift
{
    class GameLiftServerSDKWrapper;

    //! GameLift server process settings.
    struct GameLiftServerProcessDesc
    {
        AZStd::vector<AZStd::string> m_logPaths; //!< Log paths the servers will write to. Both relative to the game root folder and absolute paths supported.

        int m_port = 0; //!< The port the server will be listening on.
    };

    //! Manage the server process for hosting game sessions via GameLiftServerSDK.
    class AWSGameLiftServerManager
    {
    public:
        AWSGameLiftServerManager();
        virtual ~AWSGameLiftServerManager();

        //! Initialize GameLift API client by calling InitSDK().
        //! @return Whether the initialization is successful.
        bool InitializeGameLiftServerSDK();

        //! Notify GameLift that the server process is ready to host a game session.
        //! @param desc GameLift server process settings.
        //! @return Whether the ProcessReady notification is sent to GameLift.
        bool NotifyGameLiftProcessReady(const GameLiftServerProcessDesc& desc);

        //! Handle the destroy game session request.
        //! @return Whether the game session and the server process are shut down.
        bool ShutDownGameSession();

    protected:
        void SetGameLiftServerSDKWrapper(AZStd::unique_ptr<GameLiftServerSDKWrapper> gameLiftServerSDKWrapper);

    private:
        //! Callback function that the GameLift service invokes to activate a new game session.
        void OnStartGameSession(Aws::GameLift::Server::Model::GameSession myGameSession);

        //! Callback function that the GameLift service invokes to pass an updated game session object to the server process.
        void OnUpdateGameSession();

        //! Callback function that the server process or GameLift service invokes to force the server process to shut down.
        //! @return whether the server process is terminated and the ProcessEnding notification is sent to GameLift.
        bool OnProcessTerminate();

        //! Callback function that the GameLift service invokes to request a health status report from the server process.
        //! @return Whether the server process is healthy.
        bool OnHealthCheck();

        AZStd::unique_ptr<Aws::GameLift::GenericOutcomeCallable> m_serverProcessInitOutcome;
        AZStd::unique_ptr<GameLiftServerSDKWrapper> m_gameLiftServerSDKWrapper;
        bool m_serverSDKInitialized;
    };
} // namespace AWSGameLift
