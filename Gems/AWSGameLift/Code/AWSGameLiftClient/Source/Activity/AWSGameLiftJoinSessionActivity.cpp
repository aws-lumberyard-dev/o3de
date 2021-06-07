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

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Session/ISessionHandlingRequests.h>

#include <Activity/AWSGameLiftJoinSessionActivity.h>

namespace AWSGameLift
{
    namespace JoinSessionActivity
    {
        Aws::GameLift::Model::CreatePlayerSessionRequest BuildAWSGameLiftCreatePlayerSessionRequest(
            const AWSGameLiftJoinSessionRequest& joinSessionRequest)
        {
            Aws::GameLift::Model::CreatePlayerSessionRequest request;
            request.SetPlayerData(joinSessionRequest.m_playerData.c_str());
            request.SetPlayerId(joinSessionRequest.m_playerId.c_str());
            request.SetGameSessionId(joinSessionRequest.m_sessionId.c_str());
            return request;
        }

        AzFramework::SessionConnectionConfig BuildSessionConnectionConfig(
            const Aws::GameLift::Model::CreatePlayerSessionOutcome& createPlayerSessionOutcome)
        {
            AzFramework::SessionConnectionConfig sessionConnectionConfig;
            auto createPlayerSessionResult = createPlayerSessionOutcome.GetResult();
            // TODO: AWSNativeSDK needs to be updated to support this attribute, and it is a must have for TLS certificate enabled fleet
            //sessionConnectionConfig.m_dnsName = createPlayerSessionResult.GetPlayerSession().GetDnsName().c_str();
            sessionConnectionConfig.m_ipAddress = createPlayerSessionResult.GetPlayerSession().GetIpAddress().c_str();
            sessionConnectionConfig.m_playerSessionId = createPlayerSessionResult.GetPlayerSession().GetPlayerSessionId().c_str();
            sessionConnectionConfig.m_port = createPlayerSessionResult.GetPlayerSession().GetPort();
            return sessionConnectionConfig;
        }

        Aws::GameLift::Model::CreatePlayerSessionOutcome CreatePlayerSession(
            const Aws::GameLift::GameLiftClient& gameliftClient,
            const AWSGameLiftJoinSessionRequest& joinSessionRequest,
            const AWSErrorCallback& errorCallback)
        {
            Aws::GameLift::Model::CreatePlayerSessionRequest request =
                BuildAWSGameLiftCreatePlayerSessionRequest(joinSessionRequest);
            auto createPlayerSessionOutcome = gameliftClient.CreatePlayerSession(request);
            if (!createPlayerSessionOutcome.IsSuccess())
            {
                errorCallback(createPlayerSessionOutcome.GetError());
            }
            return createPlayerSessionOutcome;
        }

        bool RequestPlayerJoinSession(
            const Aws::GameLift::Model::CreatePlayerSessionOutcome& createPlayerSessionOutcome,
            const AZStd::function<void()>& errorCallback)
        {
            bool result = false;
            if (createPlayerSessionOutcome.IsSuccess())
            {
                auto clientRequestHandler = AZ::Interface<AzFramework::ISessionHandlingClientRequests>::Get();
                if (clientRequestHandler)
                {
                    AzFramework::SessionConnectionConfig sessionConnectionConfig =
                        BuildSessionConnectionConfig(createPlayerSessionOutcome);
                    result = clientRequestHandler->RequestPlayerJoinSession(sessionConnectionConfig);
                }
                else
                {
                    errorCallback();
                }
            }
            return result;
        }

        bool ValidateJoinSessionRequest(const AzFramework::JoinSessionRequest& joinSessionRequest)
        {
            auto gameliftJoinSessionRequest = azrtti_cast<const AWSGameLiftJoinSessionRequest*>(&joinSessionRequest);
            return gameliftJoinSessionRequest &&
                !gameliftJoinSessionRequest->m_playerId.empty() && !gameliftJoinSessionRequest->m_sessionId.empty();
        }
    } // namespace JoinSessionActivity
} // namespace AWSGameLift
