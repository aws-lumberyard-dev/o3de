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

#include <AzCore/std/string/string.h>

namespace AWSGameLift
{
    /* Wrapper to use to GameLift Server SDK.
     */
    class GameLiftServerSDKWrapper
    {
    public:
        GameLiftServerSDKWrapper() = default;
        virtual ~GameLiftServerSDKWrapper() = default;

        //! Reports to GameLift that the server process is now ready to receive player sessions.
        //! Should be called once all GameSession initialization has finished.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome ActivateGameSession();

        //! Initializes the GameLift SDK.
        //! Should be called when the server starts, before any GameLift-dependent initialization happens.
        //! @return If successful, returns an InitSdkOutcome object indicating that the server process is ready to call ProcessReady().
        virtual Aws::GameLift::Server::InitSDKOutcome InitSDK();

        //! Notifies the GameLift service that the server process is ready to host game sessions.
        //! @param processParameters A ProcessParameters object communicating the names of callback methods, port number and game
        //! session-specific log files about the server process.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome ProcessReady(const Aws::GameLift::Server::ProcessParameters& processParameters);

        //! Notifies the GameLift service that the server process is shutting down.
        //! @return Returns a generic outcome consisting of success or failure with an error message.
        virtual Aws::GameLift::GenericOutcome ProcessEnding();

        //! Returns the time that a server process is scheduled to be shut down.
        //! @return Timestamp using the UTC ISO8601 format.
        virtual AZStd::string GetTerminationTime();
    };
} // namespace AWSGameLift
