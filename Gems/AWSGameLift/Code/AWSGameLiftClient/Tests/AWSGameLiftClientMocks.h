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

#include <AzTest/AzTest.h>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/Outcome.h>
#include <aws/gamelift/GameLiftClient.h>
#include <aws/gamelift/GameLiftErrors.h>
#include <aws/gamelift/model/CreateGameSessionRequest.h>
#include <aws/gamelift/model/CreateGameSessionResult.h>

using namespace Aws::GameLift;

class GameLiftClientMock
    : public GameLiftClient
{
public:
    GameLiftClientMock()
        : GameLiftClient(Aws::Auth::AWSCredentials())
    {
    }

    MOCK_CONST_METHOD1(CreateGameSession, Model::CreateGameSessionOutcome(const Model::CreateGameSessionRequest& request));
};
