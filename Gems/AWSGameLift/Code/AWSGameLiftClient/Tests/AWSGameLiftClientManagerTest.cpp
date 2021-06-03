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

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <Credential/AWSCredentialBus.h>
#include <ResourceMapping/AWSResourceMappingBus.h>

#include <AWSCoreBus.h>
#include <AWSGameLiftClientFixture.h>
#include <AWSGameLiftClientManager.h>
#include <AWSGameLiftClientMocks.h>

#include <aws/gamelift/GameLiftClient.h>

using namespace AWSGameLift;

class AWSResourceMappingRequestsHandlerMock
    : public AWSCore::AWSResourceMappingRequestBus::Handler
{
public:
    AWSResourceMappingRequestsHandlerMock()
    {
        AWSCore::AWSResourceMappingRequestBus::Handler::BusConnect();
    }

    ~ AWSResourceMappingRequestsHandlerMock()
    {
        AWSCore::AWSResourceMappingRequestBus::Handler::BusDisconnect();
    }

    MOCK_CONST_METHOD0(GetDefaultRegion, AZStd::string());
    MOCK_CONST_METHOD0(GetDefaultAccountId, AZStd::string());
    MOCK_CONST_METHOD1(GetResourceAccountId, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD1(GetResourceNameId, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD1(GetResourceRegion, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD1(GetResourceType, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD1(GetServiceUrlByServiceName, AZStd::string(const AZStd::string&));
    MOCK_CONST_METHOD2(GetServiceUrlByRESTApiIdAndStage, AZStd::string(const AZStd::string&, const AZStd::string&));
    MOCK_METHOD1(ReloadConfigFile, void(bool));
};

class AWSCredentialRequestsHandlerMock
    : public AWSCore::AWSCredentialRequestBus::Handler
{
public:
    AWSCredentialRequestsHandlerMock()
    {
        AWSCore::AWSCredentialRequestBus::Handler::BusConnect();
    }

    ~AWSCredentialRequestsHandlerMock()
    {
        AWSCore::AWSCredentialRequestBus::Handler::BusDisconnect();
    }

    MOCK_CONST_METHOD0(GetCredentialHandlerOrder, int());
    MOCK_METHOD0(GetCredentialsProvider, std::shared_ptr<Aws::Auth::AWSCredentialsProvider>());
};

class AWSCoreRequestsHandlerMock
    : public AWSCore::AWSCoreRequestBus::Handler
{
public:
    AWSCoreRequestsHandlerMock()
    {
        AWSCore::AWSCoreRequestBus::Handler::BusConnect();
    }

    ~AWSCoreRequestsHandlerMock()
    {
        AWSCore::AWSCoreRequestBus::Handler::BusDisconnect();
    }

    MOCK_METHOD0(GetDefaultJobContext, AZ::JobContext*());
    MOCK_METHOD0(GetDefaultConfig, AWSCore::AwsApiJobConfig*());
};

class TestAWSGameLiftClientManager
    : public AWSGameLiftClientManager
{
public:
    TestAWSGameLiftClientManager()
    {
        m_gameliftClientMockPtr = nullptr;
    }
    ~TestAWSGameLiftClientManager()
    {
        m_gameliftClientMockPtr = nullptr;
    }

    void SetUpMockClient()
    {
        AZStd::unique_ptr<GameLiftClientMock> gameliftClientMock = AZStd::make_unique<GameLiftClientMock>();
        m_gameliftClientMockPtr = gameliftClientMock.get();
        SetGameLiftClient(AZStd::move(gameliftClientMock));
    }

    GameLiftClientMock* m_gameliftClientMockPtr;
};

class AWSGameLiftClientManagerTest
    : public AWSGameLiftClientFixture
{
protected:
    void SetUp() override
    {
        AWSGameLiftClientFixture::SetUp();

        m_gameliftClientManager = AZStd::make_unique<TestAWSGameLiftClientManager>();
        m_gameliftClientManager->SetUpMockClient();
        m_gameliftClientManager->ActivateManager();
    }

    void TearDown() override
    {
        m_gameliftClientManager->DeactivateManager();
        m_gameliftClientManager.reset();

        AWSGameLiftClientFixture::TearDown();
    }

public:
    AZStd::unique_ptr<TestAWSGameLiftClientManager> m_gameliftClientManager;
};

TEST_F(AWSGameLiftClientManagerTest, ConfigureGameLiftClient_CallWithoutRegion_GetFalseAsResult)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = m_gameliftClientManager->ConfigureGameLiftClient("");
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftClientManagerTest, ConfigureGameLiftClient_CallWithoutCredential_GetFalseAsResult)
{
    AWSResourceMappingRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultRegion()).Times(1).WillOnce(::testing::Return("us-west-2"));
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto result = m_gameliftClientManager->ConfigureGameLiftClient("");
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(result);
}

TEST_F(AWSGameLiftClientManagerTest, ConfigureGameLiftClient_CallWithRegionAndCredential_GetTrueAsResult)
{
    AWSCredentialRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetCredentialsProvider())
        .Times(1)
        .WillOnce(::testing::Return(std::make_shared<Aws::Auth::SimpleAWSCredentialsProvider>("dummyAccess", "dummySecret", "")));
    auto result = m_gameliftClientManager->ConfigureGameLiftClient("us-west-2");
    EXPECT_TRUE(result);
}

TEST_F(AWSGameLiftClientManagerTest, CreatePlayerId_CreateWithoutBracketsOrDashes_GetExpectedResult)
{
    auto result = m_gameliftClientManager->CreatePlayerId(false, false);
    EXPECT_FALSE(result.starts_with("{"));
    EXPECT_FALSE(result.ends_with("}"));
    EXPECT_FALSE(result.contains("-"));
}

TEST_F(AWSGameLiftClientManagerTest, CreatePlayerId_CreateWithBrackets_GetExpectedResult)
{
    auto result = m_gameliftClientManager->CreatePlayerId(true, false);
    EXPECT_TRUE(result.starts_with("{"));
    EXPECT_TRUE(result.ends_with("}"));
    EXPECT_FALSE(result.contains("-"));
}

TEST_F(AWSGameLiftClientManagerTest, CreatePlayerId_CreateWithDashes_GetExpectedResult)
{
    auto result = m_gameliftClientManager->CreatePlayerId(false, true);
    EXPECT_FALSE(result.starts_with("{"));
    EXPECT_FALSE(result.ends_with("}"));
    EXPECT_TRUE(result.contains("-"));
}

TEST_F(AWSGameLiftClientManagerTest, CreatePlayerId_CreateWithBracketsAndDashes_GetExpectedResult)
{
    auto result = m_gameliftClientManager->CreatePlayerId(true, true);
    EXPECT_TRUE(result.starts_with("{"));
    EXPECT_TRUE(result.ends_with("}"));
    EXPECT_TRUE(result.contains("-"));
}

TEST_F(AWSGameLiftClientManagerTest, CreateSession_CallWithoutClientSetup_GetEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->ConfigureGameLiftClient("");
    auto response = m_gameliftClientManager->CreateSession(AzFramework::CreateSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(2); // capture 2 error message
    EXPECT_TRUE(response == "");
}

TEST_F(AWSGameLiftClientManagerTest, CreateSession_CallWithInvalidRequest_GetEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto response = m_gameliftClientManager->CreateSession(AzFramework::CreateSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_TRUE(response == "");
}

TEST_F(AWSGameLiftClientManagerTest, CreateSession_CallWithValidRequest_GetSuccessOutcome)
{
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    Aws::GameLift::Model::CreateGameSessionResult result;
    result.SetGameSession(Aws::GameLift::Model::GameSession());
    Aws::GameLift::Model::CreateGameSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreateGameSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    m_gameliftClientManager->CreateSession(request);
}

TEST_F(AWSGameLiftClientManagerTest, CreateSession_CallWithValidRequest_GetErrorOutcome)
{
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::CreateGameSessionOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreateGameSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->CreateSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionAsync_CallWithoutClientSetup_GetNotificationWithEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->ConfigureGameLiftClient("");
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(AZStd::string())).Times(1);
    m_gameliftClientManager->CreateSessionAsync(AzFramework::CreateSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(2); // capture 2 error message
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionAsync_CallWithInvalidRequest_GetNotificationWithEmptyResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(AZStd::string())).Times(1);
    m_gameliftClientManager->CreateSessionAsync(AzFramework::CreateSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionAsync_CallWithValidRequest_GetNotificationWithSuccessOutcome)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    Aws::GameLift::Model::CreateGameSessionResult result;
    result.SetGameSession(Aws::GameLift::Model::GameSession());
    Aws::GameLift::Model::CreateGameSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreateGameSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(::testing::_)).Times(1);
    m_gameliftClientManager->CreateSessionAsync(request);
}

TEST_F(AWSGameLiftClientManagerTest, CreateSessionAsync_CallWithValidRequest_GetNotificationWithErrorOutcome)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftCreateSessionRequest request;
    request.m_aliasId = "dummyAlias";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::CreateGameSessionOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreateGameSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnCreateSessionAsyncComplete(AZStd::string(""))).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->CreateSessionAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithoutClientSetup_GetFalseResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->ConfigureGameLiftClient("");
    auto response = m_gameliftClientManager->JoinSession(AzFramework::JoinSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(2); // capture 2 error message
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithInvalidRequest_GetFalseResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto response = m_gameliftClientManager->JoinSession(AzFramework::JoinSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithValidRequestButNoRequestHandler_GetSuccessOutcomeButFalseResponse)
{
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto response = m_gameliftClientManager->JoinSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithValidRequest_GetErrorOutcomeAndFalseResponse)
{
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    AZ_TEST_START_TRACE_SUPPRESSION;
    auto response = m_gameliftClientManager->JoinSession(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithValidRequestAndRequestHandler_GetSuccessOutcomeButFalseResponse)
{
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerJoinSession(::testing::_)).Times(1).WillOnce(::testing::Return(false));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    auto response = m_gameliftClientManager->JoinSession(request);
    EXPECT_FALSE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSession_CallWithValidRequestAndRequestHandler_GetSuccessOutcomeAndTrueResponse)
{
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerJoinSession(::testing::_)).Times(1).WillOnce(::testing::Return(true));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    auto response = m_gameliftClientManager->JoinSession(request);
    EXPECT_TRUE(response);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithoutClientSetup_GetNotificationWithFalseResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->ConfigureGameLiftClient("");
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    m_gameliftClientManager->JoinSessionAsync(AzFramework::JoinSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(2); // capture 2 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithInvalidRequest_GetNotificationWithFalseResponse)
{
    AZ_TEST_START_TRACE_SUPPRESSION;
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    m_gameliftClientManager->JoinSessionAsync(AzFramework::JoinSessionRequest());
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithValidRequestButNoRequestHandler_GetSuccessOutcomeButNotificationWithFalseResponse)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->JoinSessionAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithValidRequest_GetErrorOutcomeAndNotificationWithFalseResponse)
{
    AWSCoreRequestsHandlerMock handlerMock;
    EXPECT_CALL(handlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::Client::AWSError<Aws::GameLift::GameLiftErrors> error;
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(error);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    AZ_TEST_START_TRACE_SUPPRESSION;
    m_gameliftClientManager->JoinSessionAsync(request);
    AZ_TEST_STOP_TRACE_SUPPRESSION(1); // capture 1 error message
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithValidRequestAndRequestHandler_GetSuccessOutcomeButNotificationWithFalseResponse)
{
    AWSCoreRequestsHandlerMock coreHandlerMock;
    EXPECT_CALL(coreHandlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerJoinSession(::testing::_)).Times(1).WillOnce(::testing::Return(false));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(false)).Times(1);
    m_gameliftClientManager->JoinSessionAsync(request);
}

TEST_F(AWSGameLiftClientManagerTest, JoinSessionAsync_CallWithValidRequestAndRequestHandler_GetSuccessOutcomeAndNotificationWithTrueResponse)
{
    AWSCoreRequestsHandlerMock coreHandlerMock;
    EXPECT_CALL(coreHandlerMock, GetDefaultJobContext()).Times(1).WillOnce(::testing::Return(m_jobContext.get()));
    SessionHandlingClientRequestsMock handlerMock;
    EXPECT_CALL(handlerMock, RequestPlayerJoinSession(::testing::_)).Times(1).WillOnce(::testing::Return(true));
    AWSGameLiftJoinSessionRequest request;
    request.m_sessionId = "dummySessionId";
    request.m_playerId = "dummyPlayerId";
    Aws::GameLift::Model::CreatePlayerSessionResult result;
    result.SetPlayerSession(Aws::GameLift::Model::PlayerSession());
    Aws::GameLift::Model::CreatePlayerSessionOutcome outcome(result);
    EXPECT_CALL(*(m_gameliftClientManager->m_gameliftClientMockPtr), CreatePlayerSession(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(outcome));
    SessionAsyncRequestNotificationsHandlerMock sessionHandlerMock;
    EXPECT_CALL(sessionHandlerMock, OnJoinSessionAsyncComplete(true)).Times(1);
    m_gameliftClientManager->JoinSessionAsync(request);
}
