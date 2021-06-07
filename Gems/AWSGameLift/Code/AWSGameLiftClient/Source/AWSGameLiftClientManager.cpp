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

#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzFramework/Session/SessionConfig.h>

#include <AWSCoreBus.h>
#include <Credential/AWSCredentialBus.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <AWSGameLiftClientManager.h>
#include <Activity/AWSGameLiftCreateSessionActivity.h>
#include <Activity/AWSGameLiftJoinSessionActivity.h>

#include <aws/core/auth/AWSCredentialsProvider.h>

namespace AWSGameLift
{
#if defined(AWSGAMELIFT_DEV)
    AZ_CVAR(AZ::CVarFixedString, cl_gameliftLocalEndpoint, "", nullptr, AZ::ConsoleFunctorFlags::Null, "The local endpoint to test with GameLiftLocal SDK.");
#endif

    AWSGameLiftClientManager::AWSGameLiftClientManager()
    {
        m_gameliftClient.reset();
    }

    void AWSGameLiftClientManager::ActivateManager()
    {
        AZ::Interface<IAWSGameLiftRequests>::Register(this);
        AWSGameLiftRequestBus::Handler::BusConnect();

        AZ::Interface<AzFramework::ISessionAsyncRequests>::Register(this);
        AWSGameLiftSessionAsyncRequestBus::Handler::BusConnect();

        AZ::Interface<AzFramework::ISessionRequests>::Register(this);
        AWSGameLiftSessionRequestBus::Handler::BusConnect();
    }

    void AWSGameLiftClientManager::DeactivateManager()
    {
        AWSGameLiftSessionRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzFramework::ISessionRequests>::Unregister(this);

        AWSGameLiftSessionAsyncRequestBus::Handler::BusDisconnect();
        AZ::Interface<AzFramework::ISessionAsyncRequests>::Unregister(this);

        AWSGameLiftRequestBus::Handler::BusDisconnect();
        AZ::Interface<IAWSGameLiftRequests>::Unregister(this);
    }

    bool AWSGameLiftClientManager::ConfigureGameLiftClient(const AZStd::string& region)
    {
        m_gameliftClient.reset();

        Aws::Client::ClientConfiguration clientConfig;
        // Set up client endpoint or region
        AZStd::string localEndpoint = "";
#if defined(AWSGAMELIFT_DEV)
        localEndpoint = static_cast<AZ::CVarFixedString>(cl_gameliftLocalEndpoint);
#endif
        if (!localEndpoint.empty())
        {
            // The attribute needs to override to interact with GameLiftLocal
            clientConfig.endpointOverride = localEndpoint.c_str();
        }
        else if (!region.empty())
        {
            clientConfig.region = region.c_str();
        }
        else
        {
            AZStd::string clientRegion;
            AWSCore::AWSResourceMappingRequestBus::BroadcastResult(clientRegion, &AWSCore::AWSResourceMappingRequests::GetDefaultRegion);
            if (clientRegion.empty())
            {
                AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientRegionMissingErrorMessage);
                return false;
            }
            clientConfig.region = clientRegion.c_str();
        }

        // Fetch AWS credential for client
        AWSCore::AWSCredentialResult credentialResult;
        AWSCore::AWSCredentialRequestBus::BroadcastResult(credentialResult, &AWSCore::AWSCredentialRequests::GetCredentialsProvider);
        if (!localEndpoint.empty())
        {
            credentialResult.result = std::make_shared<Aws::Auth::AnonymousAWSCredentialsProvider>();
        }
        else if (!credentialResult.result)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientCredentialMissingErrorMessage);
            return false;
        }
        m_gameliftClient = AZStd::make_unique<Aws::GameLift::GameLiftClient>(credentialResult.result, clientConfig);
        return true;
    }

    AZStd::string AWSGameLiftClientManager::CreatePlayerId(bool includeBrackets, bool includeDashes)
    {
        return AZ::Uuid::CreateRandom().ToString<AZStd::string>(includeBrackets, includeDashes);
    }

    AZStd::string AWSGameLiftClientManager::CreateSession(const AzFramework::CreateSessionRequest& createSessionRequest)
    {
        AZStd::string result = "";
        if (!m_gameliftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else
        {
            if (CreateSessionActivity::ValidateCreateSessionRequest(createSessionRequest))
            {
                AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting CreateGameSession against Amazon GameLift service...");
                auto& gameliftCreateSessionRequest = static_cast<const AWSGameLiftCreateSessionRequest&>(createSessionRequest);
                result = CreateSessionActivity::CreateSession(*(m_gameliftClient.get()), gameliftCreateSessionRequest,
                    [](const Aws::Client::AWSError<Aws::GameLift::GameLiftErrors>& error) {
                        AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientErrorMessageTemplate,
                            error.GetExceptionName().c_str(), error.GetMessage().c_str());
                    });
            }
            else
            {
                AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftCreateSessionRequestInvalidErrorMessage);
            }
        }

        return result;
    }

    void AWSGameLiftClientManager::CreateSessionAsync(const AzFramework::CreateSessionRequest& createSessionRequest)
    {
        if (!m_gameliftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
            AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                &AzFramework::SessionAsyncRequestNotifications::OnCreateSessionAsyncComplete, "");
        }
        else
        {
            if (CreateSessionActivity::ValidateCreateSessionRequest(createSessionRequest))
            {
                AZ::JobContext* jobContext = nullptr;
                AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
                auto& gameliftCreateSessionRequest = static_cast<const AWSGameLiftCreateSessionRequest&>(createSessionRequest);

                AZ::Job* createSessionJob = AZ::CreateJobFunction(
                    [this, gameliftCreateSessionRequest]() {
                        AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting CreateGameSession against Amazon GameLift service asynchronously...");
                        AZStd::string result = CreateSessionActivity::CreateSession(
                            *(m_gameliftClient.get()), gameliftCreateSessionRequest,
                            [](const Aws::Client::AWSError<Aws::GameLift::GameLiftErrors>& error) {
                                AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientErrorMessageTemplate,
                                    error.GetExceptionName().c_str(), error.GetMessage().c_str());
                            });
                        AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                            &AzFramework::SessionAsyncRequestNotifications::OnCreateSessionAsyncComplete, result);
                    },
                    true, jobContext);
                createSessionJob->Start();
            }
            else
            {
                AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftCreateSessionRequestInvalidErrorMessage);
                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnCreateSessionAsyncComplete, "");
            }
        }
    }

    bool AWSGameLiftClientManager::JoinSession(const AzFramework::JoinSessionRequest& joinSessionRequest)
    {
        bool result = false;
        if (!m_gameliftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
        }
        else
        {
            if (JoinSessionActivity::ValidateJoinSessionRequest(joinSessionRequest))
            {
                AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting CreatePlayerSession call against Amazon GameLift service...");
                auto& gameliftJoinSessionRequest = static_cast<const AWSGameLiftJoinSessionRequest&>(joinSessionRequest);
                auto createPlayerSessionOutcome = JoinSessionActivity::CreatePlayerSession(
                    *(m_gameliftClient.get()), gameliftJoinSessionRequest,
                    [](const Aws::Client::AWSError<Aws::GameLift::GameLiftErrors>& error) {
                        AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientErrorMessageTemplate,
                            error.GetExceptionName().c_str(), error.GetMessage().c_str());
                    });

                AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting player to connect to game session...");
                result = JoinSessionActivity::RequestPlayerJoinSession(createPlayerSessionOutcome, []() {
                    AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftJoinSessionMissingRequestHandlerErrorMessage);
                });
            }
            else
            {
                AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftJoinSessionRequestInvalidErrorMessage);
            }
        }

        return result;
    }

    void AWSGameLiftClientManager::JoinSessionAsync(const AzFramework::JoinSessionRequest& joinSessionRequest)
    {
        if (!m_gameliftClient)
        {
            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientMissingErrorMessage);
            AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                &AzFramework::SessionAsyncRequestNotifications::OnJoinSessionAsyncComplete, false);
        }
        else
        {
            if (JoinSessionActivity::ValidateJoinSessionRequest(joinSessionRequest))
            {
                AZ::JobContext* jobContext = nullptr;
                AWSCore::AWSCoreRequestBus::BroadcastResult(jobContext, &AWSCore::AWSCoreRequests::GetDefaultJobContext);
                auto& gameliftJoinSessionRequest = static_cast<const AWSGameLiftJoinSessionRequest&>(joinSessionRequest);

                AZ::Job* joinSessionJob = AZ::CreateJobFunction(
                    [this, gameliftJoinSessionRequest]() {
                        AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting CreatePlayerSession call against Amazon GameLift service asynchronously...");
                        auto createPlayerSessionOutcome = JoinSessionActivity::CreatePlayerSession(
                            *(m_gameliftClient.get()), gameliftJoinSessionRequest,
                            [](const Aws::Client::AWSError<Aws::GameLift::GameLiftErrors>& error) {
                                AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftClientErrorMessageTemplate,
                                    error.GetExceptionName().c_str(), error.GetMessage().c_str());
                            });

                        AZ_TracePrintf(AWSGameLiftClientManagerName, "Requesting player to connect to game session asynchronously...");
                        bool result = JoinSessionActivity::RequestPlayerJoinSession(createPlayerSessionOutcome, []() {
                            AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftJoinSessionMissingRequestHandlerErrorMessage);
                        });
                        AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                            &AzFramework::SessionAsyncRequestNotifications::OnJoinSessionAsyncComplete, result);
                    },
                    true, jobContext);
                joinSessionJob->Start();
            }
            else
            {
                AZ_Error(AWSGameLiftClientManagerName, false, AWSGameLiftJoinSessionRequestInvalidErrorMessage);
                AzFramework::SessionAsyncRequestNotificationBus::Broadcast(
                    &AzFramework::SessionAsyncRequestNotifications::OnJoinSessionAsyncComplete, false);
            }
        }
    }

    void AWSGameLiftClientManager::LeaveSession()
    {
        // TODO: Add implementation for leave session
    }

    void AWSGameLiftClientManager::LeaveSessionAsync()
    {
        // TODO: Add implementation for leave session
    }

    AzFramework::SearchSessionsResponse AWSGameLiftClientManager::SearchSessions(
        const AzFramework::SearchSessionsRequest& searchSessionsRequest) const
    {
        // TODO: Add implementation for search sessions
        AZ_UNUSED(searchSessionsRequest);
        return AzFramework::SearchSessionsResponse();
    }

    void AWSGameLiftClientManager::SearchSessionsAsync(const AzFramework::SearchSessionsRequest& searchSessionsRequest) const
    {
        // TODO: Add implementation for search sessions
        AZ_UNUSED(searchSessionsRequest);
    }

    void AWSGameLiftClientManager::SetGameLiftClient(AZStd::unique_ptr<Aws::GameLift::GameLiftClient> gameliftClient)
    {
        m_gameliftClient.reset();
        m_gameliftClient = AZStd::move(gameliftClient);
    }
} // namespace AWSGameLift
