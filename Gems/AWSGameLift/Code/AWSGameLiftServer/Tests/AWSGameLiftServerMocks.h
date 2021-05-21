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
        AWSGameLiftServerManagerMock()
            : m_gameLiftServerSDKWrapper(AZStd::make_unique<NiceMock<GameLiftServerSDKWrapperMock>>())
            , m_gameLiftServerSDKWrapperRef(*m_gameLiftServerSDKWrapper)
        {
            SetGameLiftServerSDKWrapper(AZStd::move(m_gameLiftServerSDKWrapper));
        }

        AZStd::unique_ptr<NiceMock<GameLiftServerSDKWrapperMock>> m_gameLiftServerSDKWrapper;
        NiceMock<GameLiftServerSDKWrapperMock>& m_gameLiftServerSDKWrapperRef;

    };

    class AWSGameLiftServerSystemComponentMock : public AWSGameLift::AWSGameLiftServerSystemComponent
    {
    public:
        AWSGameLiftServerSystemComponentMock()
        {
            SetGameLiftServerManager(AZStd::make_unique<NiceMock<AWSGameLiftServerManagerMock>>());

            ON_CALL(*this, Init()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::InitMock));
            ON_CALL(*this, Activate()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::ActivateMock));
            ON_CALL(*this, Deactivate()).WillByDefault(testing::Invoke(this, &AWSGameLiftServerSystemComponentMock::DeactivateMock));
        }

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

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());

        GameLiftServerProcessDesc m_serverProcessDesc;
    };
};
