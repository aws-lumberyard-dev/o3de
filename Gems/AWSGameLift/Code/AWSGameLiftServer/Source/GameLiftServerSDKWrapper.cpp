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

#include <GameLiftServerSDKWrapper.h>

namespace AWSGameLift
{
    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::ActivateGameSession()
    {
        return Aws::GameLift::Server::ActivateGameSession();
    }

    Aws::GameLift::Server::InitSDKOutcome GameLiftServerSDKWrapper::InitSDK()
    {
        return Aws::GameLift::Server::InitSDK();
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::ProcessReady(
        const Aws::GameLift::Server::ProcessParameters& processParameters)
    {
        return Aws::GameLift::Server::ProcessReady(processParameters);
    }

    Aws::GameLift::GenericOutcome GameLiftServerSDKWrapper::ProcessEnding()
    {
        return Aws::GameLift::Server::ProcessEnding();
    }
} // namespace AWSGameLift
