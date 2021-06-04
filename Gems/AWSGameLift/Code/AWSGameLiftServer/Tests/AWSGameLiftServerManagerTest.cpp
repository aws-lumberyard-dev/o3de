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

#include <AWSGameLiftServerFixture.h>
#include <AWSGameLiftServerMocks.h>

#include <AzCore/Interface/Interface.h>
#include <AzFramework/Session/SessionConfig.h>
#include <AzFramework/Session/SessionNotifications.h>

namespace UnitTest
{
    class SessionNotificationsHandlerMock
        : public AzFramework::SessionNotificationBus::Handler
    {
    public:
        SessionNotificationsHandlerMock()
        {
            AzFramework::SessionNotificationBus::Handler::BusConnect();
        }

        ~SessionNotificationsHandlerMock()
        {
            AzFramework::SessionNotificationBus::Handler::BusDisconnect();
        }

        MOCK_METHOD0(OnSessionHealthCheck, bool());
        MOCK_METHOD1(OnCreateSessionBegin, bool(const AzFramework::SessionConfig&));
        MOCK_METHOD0(OnDestroySessionBegin, bool());
    };

    class GameLiftServerManagerTest
        : public AWSGameLiftServerFixture
    {
    public:
        void SetUp() override
        {
            AWSGameLiftServerFixture::SetUp();

            GameLiftServerProcessDesc serverDesc;
            m_serverManager = AZStd::make_unique<NiceMock<AWSGameLiftServerManagerMock>>();
        }

        void TearDown() override
        {
            m_serverManager.reset();

            AWSGameLiftServerFixture::TearDown();
        }

        AZStd::unique_ptr<NiceMock<AWSGameLiftServerManagerMock>> m_serverManager;
    };

    TEST_F(GameLiftServerManagerTest, InitializeGameLiftServerSDK_InitializeTwice_InitSDKCalledOnce)
    {
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), InitSDK()).Times(1);

        EXPECT_TRUE(m_serverManager->InitializeGameLiftServerSDK());

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_serverManager->InitializeGameLiftServerSDK());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, NotifyGameLiftProcessReady_SDKNotInitialized_FailToNotifyGameLift)
    {
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessReady(testing::_)).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc()));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, NotifyGameLiftProcessReady_SDKInitialized_ProcessReadyNotificationSent)
    {
        EXPECT_TRUE(m_serverManager->InitializeGameLiftServerSDK());

        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessReady(testing::_)).Times(1);

        EXPECT_TRUE(m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc()));
    }

    TEST_F(GameLiftServerManagerTest, NotifyGameLiftProcessReady_ProcessReadyFails_TerminationNotificationSent)
    {
        EXPECT_TRUE(m_serverManager->InitializeGameLiftServerSDK());

        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessReady(testing::_))
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome()));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc()));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, OnProcessTerminate_OnDestroySessionBeginReturnsFalse_FailToNotifyGameLift)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc());
        if (!AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Get())
        {
            AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Register(m_serverManager.get());
        }

        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(false));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onProcessTerminateFunc();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        EXPECT_FALSE(AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Get());
    }

    TEST_F(GameLiftServerManagerTest, OnProcessTerminate_OnDestroySessionBeginReturnsTrue_TerminationNotificationSent)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc());
        if (!AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Get())
        {
            AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Register(m_serverManager.get());
        }

        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);

        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onProcessTerminateFunc();

        EXPECT_FALSE(AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Get());
    }

    TEST_F(GameLiftServerManagerTest, OnHealthCheck_OnSessionHealthCheckReturnsTrue_CallbackFunctionReturnsTrue)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc());
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnSessionHealthCheck()).Times(1).WillOnce(testing::Return(true));
        EXPECT_TRUE(m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_healthCheckFunc());
    }

    TEST_F(GameLiftServerManagerTest, OnHealthCheck_OnSessionHealthCheckReturnsFalseAndTrue_CallbackFunctionReturnsFalse)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc());
        SessionNotificationsHandlerMock handlerMock1;
        EXPECT_CALL(handlerMock1, OnSessionHealthCheck()).Times(1).WillOnce(testing::Return(false));
        SessionNotificationsHandlerMock handlerMock2;
        EXPECT_CALL(handlerMock2, OnSessionHealthCheck()).Times(1).WillOnce(testing::Return(true));
        EXPECT_FALSE(m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_healthCheckFunc());
    }

    TEST_F(GameLiftServerManagerTest, OnHealthCheck_OnSessionHealthCheckReturnsFalse_CallbackFunctionReturnsFalse)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc());
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnSessionHealthCheck()).Times(1).WillOnce(testing::Return(false));
        EXPECT_FALSE(m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_healthCheckFunc());
    }

    TEST_F(GameLiftServerManagerTest, OnStartGameSession_OnCreateSessionBeginReturnsFalse_TerminationNotificationSent)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc());
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnCreateSessionBegin(testing::_)).Times(1).WillOnce(testing::Return(false));
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onStartGameSessionFunc(Aws::GameLift::Server::Model::GameSession());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, OnStartGameSession_ActivateGameSessionSucceeds_RegisterAsHandler)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc());
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnCreateSessionBegin(testing::_)).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ActivateGameSession())
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome(nullptr)));
        Aws::GameLift::Server::Model::GameSession testSession;
        Aws::GameLift::Server::Model::GameProperty testProperty;
        testProperty.SetKey("testKey");
        testProperty.SetValue("testValue");
        testSession.AddGameProperties(testProperty);
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onStartGameSessionFunc(testSession);
        EXPECT_TRUE(AZ::Interface<AzFramework::ISessionHandlingServerRequests>::Get());
        m_serverManager->HandleDestroySession();
    }

    TEST_F(GameLiftServerManagerTest, OnStartGameSession_ActivateGameSessionFails_TerminationNotificationSent)
    {
        m_serverManager->InitializeGameLiftServerSDK();
        m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc());
        SessionNotificationsHandlerMock handlerMock;
        EXPECT_CALL(handlerMock, OnCreateSessionBegin(testing::_)).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(handlerMock, OnDestroySessionBegin()).Times(1).WillOnce(testing::Return(true));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ActivateGameSession())
            .Times(1)
            .WillOnce(testing::Return(Aws::GameLift::GenericOutcome()));
        EXPECT_CALL(*(m_serverManager->m_gameLiftServerSDKWrapperMockPtr), ProcessEnding()).Times(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_serverManager->m_gameLiftServerSDKWrapperMockPtr->m_onStartGameSessionFunc(Aws::GameLift::Server::Model::GameSession());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }
} // namespace UnitTest
