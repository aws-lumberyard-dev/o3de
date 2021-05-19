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

#include <AWSGameLiftServerSystemComponent.h>
#include <AWSGameLiftServerManager.h>
#include <GameLiftServerSDKWrapper.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>

using namespace Aws::GameLift;
using namespace AWSGameLift;
using testing::_;
using testing::Invoke;
using testing::Return;
using testing::NiceMock;
using testing::Eq;

namespace UnitTest
{
    class GameLiftServerSDKWrapperMock : public GameLiftServerSDKWrapper
    {
    public:
        GameLiftServerSDKWrapperMock()
        {
            GenericOutcome successOutcome(nullptr);
            Server::InitSDKOutcome sdkOutcome(nullptr);

            ON_CALL(*this, InitSDK()).WillByDefault(Return(sdkOutcome));
            ON_CALL(*this, ProcessReadyAsync(_)).WillByDefault(Invoke(this, &GameLiftServerSDKWrapperMock::ProcessReadyAsyncMock));
            ON_CALL(*this, ProcessEnding()).WillByDefault(Return(successOutcome));
        }
        MOCK_METHOD0(InitSDK, Server::InitSDKOutcome());
        MOCK_METHOD1(ProcessReadyAsync, GenericOutcomeCallable(const Server::ProcessParameters& processParameters));
        MOCK_METHOD0(ProcessEnding, GenericOutcome());

        GenericOutcomeCallable ProcessReadyAsyncMock(const Server::ProcessParameters& processParameters)
        {
            AZ_UNUSED(processParameters);

            GenericOutcome successOutcome(nullptr);

            std::promise<GenericOutcome> outcomePromise;
            outcomePromise.set_value(successOutcome);
            return outcomePromise.get_future();
        }
    };

    class AWSGameLiftServerManagerMock : public AWSGameLiftServerManager
    {
    public:
        AWSGameLiftServerManagerMock(const GameLiftServerProcessDesc desc)
            : AWSGameLiftServerManager(desc)
        {
            ON_CALL(*this, GetGameLiftServerSDKWrapper())
                .WillByDefault(Invoke(this, &AWSGameLiftServerManagerMock::GetGameLiftServerSDKWrapperMock));
        }
        ~AWSGameLiftServerManagerMock()
        {
            m_gameLiftServerSDKWrapperMock.reset();
        }

        MOCK_METHOD0(GetGameLiftServerSDKWrapper, AZStd::weak_ptr<GameLiftServerSDKWrapper>());

        AZStd::weak_ptr<GameLiftServerSDKWrapper> GetGameLiftServerSDKWrapperMock()
        {
            if (!m_gameLiftServerSDKWrapperMock)
            {
                m_gameLiftServerSDKWrapperMock = AZStd::make_shared<NiceMock<GameLiftServerSDKWrapperMock>>();
            }

            return m_gameLiftServerSDKWrapperMock;
        }

    protected:
        AZStd::shared_ptr<NiceMock<GameLiftServerSDKWrapperMock>> m_gameLiftServerSDKWrapperMock;
    };

    class AWSGameLiftServerSystemComponentMock : public AWSGameLift::AWSGameLiftServerSystemComponent
    {
    public:
        void InitMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Init();
        }

        void ActivateMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Activate();
        }

        void DeactivateMock()
        {
            AWSGameLift::AWSGameLiftServerSystemComponent::Deactivate();
        }

        AWSGameLiftServerSystemComponentMock()
        {
            ON_CALL(*this, Init()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::InitMock));
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::DeactivateMock));
            ON_CALL(*this, GetGameLiftServerManager())
                .WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::GetGameLiftServerManagerMock));
        }

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());
        MOCK_METHOD0(GetGameLiftServerManager, AZStd::weak_ptr<AWSGameLiftServerManager>());

        AZStd::weak_ptr<AWSGameLiftServerManager> GetGameLiftServerManagerMock()
        {
            if (!m_gameLiftServerManagerMock)
            {
                SetGameLiftServerProcessDesc(m_serverProcessDesc);
                m_gameLiftServerManagerMock = AZStd::make_shared<NiceMock<AWSGameLiftServerManagerMock>>(m_serverProcessDesc);
            }

            return m_gameLiftServerManagerMock;
        }

        AZStd::shared_ptr<NiceMock<AWSGameLiftServerManagerMock>> m_gameLiftServerManagerMock;
        GameLiftServerProcessDesc m_serverProcessDesc;
    };
};
