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

#include <AWSGameLiftServerMocks.h>

namespace UnitTest
{
    class GameLiftServerManagerTest
        : public ScopedAllocatorSetupFixture
    {
    public:
        void SetUp() override
        {
            ScopedAllocatorSetupFixture::SetUp();

            GameLiftServerProcessDesc serverDesc;
            m_serverManager = AZStd::make_unique<NiceMock<AWSGameLiftServerManagerMock>>();
        }

        void TearDown() override
        {
            m_serverManager.reset();

            ScopedAllocatorSetupFixture::TearDown();
        }

        AZStd::unique_ptr<NiceMock<AWSGameLiftServerManagerMock>> m_serverManager;
    };

    TEST_F(GameLiftServerManagerTest, InitializeGameLiftServerSDK_InitializeTwice_InitSDKCalledOnce)
    {
        EXPECT_CALL(m_serverManager->m_gameLiftServerSDKWrapperRef, InitSDK()).Times(1);

        EXPECT_TRUE(m_serverManager->InitializeGameLiftServerSDK());

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_serverManager->InitializeGameLiftServerSDK());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, NotifyGameLiftProcessReady_SDKNotInitialized_FailToNotifyGameLift)
    {
        EXPECT_CALL(m_serverManager->m_gameLiftServerSDKWrapperRef, ProcessReadyAsync(testing::_)).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc()));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, NotifyGameLiftProcessReady_SDKInitialized_ProcessReadyNotificationSent)
    {
        EXPECT_TRUE(m_serverManager->InitializeGameLiftServerSDK());

        EXPECT_CALL(m_serverManager->m_gameLiftServerSDKWrapperRef, ProcessReadyAsync(testing::_)).Times(1);

        EXPECT_TRUE(m_serverManager->NotifyGameLiftProcessReady(GameLiftServerProcessDesc()));
    }

    TEST_F(GameLiftServerManagerTest, TerminateServerProcess_SDKNotInitialized_FailToNotifyGameLift)
    {
        EXPECT_CALL(m_serverManager->m_gameLiftServerSDKWrapperRef, ProcessEnding()).Times(0);

        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_serverManager->ShutDownGameSession());
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(GameLiftServerManagerTest, TerminateServerProcess_SDKInitialized_TerminationNotificationSent)
    {
        EXPECT_TRUE(m_serverManager->InitializeGameLiftServerSDK());

        EXPECT_CALL(m_serverManager->m_gameLiftServerSDKWrapperRef, ProcessEnding()).Times(1);

        EXPECT_TRUE(m_serverManager->ShutDownGameSession());
    }
} // namespace UnitTest
