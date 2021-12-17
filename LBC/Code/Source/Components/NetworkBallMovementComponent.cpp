/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Components/NetworkBallMovementComponent.h>

#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <LBC/LBCBus.h>
#include <AzCore/Time/ITime.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>

namespace LBC
{
    NetworkBallMovementComponentController::NetworkBallMovementComponentController(NetworkBallMovementComponent& parent)
        : NetworkBallMovementComponentControllerBase(parent)
    {
        ;
    }

    void NetworkBallMovementComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (IsAutonomous())
        {
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(ImpulseEventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusConnect(Impulse2EventId);
        }
    }

    void NetworkBallMovementComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (IsAutonomous())
        {
            AZ::TickBus::Handler::BusDisconnect();
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(Impulse2EventId);
            StartingPointInput::InputEventNotificationBus::MultiHandler::BusDisconnect(ImpulseEventId);
        }
    }

    void NetworkBallMovementComponentController::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        m_power = std::min<float>(m_power + deltaTime * 1000, 700);
    }

    void NetworkBallMovementComponentController::CreateInput(Multiplayer::NetworkInput& input, [[maybe_unused]] float deltaTime)
    {
        // Inputs for your own component always exist
        NetworkBallMovementComponentNetworkInput* playerInput = input.FindComponentInput<NetworkBallMovementComponentNetworkInput>();

        if (m_impulseReleased || m_impulse2Released)
        {
            AZ::EntityId entityId = m_impulseReleased ? AZ::Interface<LBCRequests>::Get()->GetRedBallEntityId()
                                                     : AZ::Interface<LBCRequests>::Get()->GetBlueBallEntityId();
            playerInput->m_mouse = m_impulseReleased ? "Left" : "Right";

            AZ::Vector3 worldTranslation;
            AZ::TransformBus::EventResult(worldTranslation, entityId, &AZ::TransformBus::Events::GetWorldTranslation);

            AZ::EntityId activeCameraId;
            Camera::CameraSystemRequestBus::BroadcastResult(activeCameraId, &Camera::CameraSystemRequests::GetActiveCamera);

            AZ::Vector3 localTranslation;
            AZ::TransformBus::EventResult(localTranslation, activeCameraId, &AZ::TransformBus::Events::GetLocalTranslation);

            playerInput->m_translationDiff = worldTranslation - localTranslation;
            playerInput->m_translationDiff.SetZ(0);
            playerInput->m_translationDiff.Normalize();

            playerInput->m_power = m_power;

            m_impulseReleased = false;
            m_impulse2Released = false;
        }
        else
        {
            playerInput->m_translationDiff = AZ::Vector3(0, 0, 0);
        }

        // Just a note for anyone who is super confused by this, ResetCount is a predictable network property, it gets set on the client
        // through correction packets
        playerInput->m_resetCount = GetNetworkTransformComponentController()->GetResetCount();
    }

    void NetworkBallMovementComponentController::ProcessInput(Multiplayer::NetworkInput& input, [[maybe_unused]] float deltaTime)
    {
        // If the input reset count doesn't match the state's reset count it can mean two things:
        //  1) On the server: we were reset and we are now receiving inputs from the client for an old reset count
        //  2) On the client: we were reset and we are replaying old inputs after being corrected
        // In both cases we don't want to process these inputs
        NetworkBallMovementComponentNetworkInput* playerInput = input.FindComponentInput<NetworkBallMovementComponentNetworkInput>();
        if (playerInput->m_resetCount != GetNetworkTransformComponentController()->GetResetCount())
        {
            return;
        }

        // Update velocity
        UpdateForce(*playerInput);
        AZ::EntityId ballEntityId = AZ::EntityId();
        if (playerInput->m_mouse == "Left")
        {
            ballEntityId = AZ::Interface<LBCRequests>::Get()->GetRedBallEntityId();
        }
        else if (playerInput->m_mouse == "Right")
        {
            ballEntityId = AZ::Interface<LBCRequests>::Get()->GetBlueBallEntityId();
        }

        if (ballEntityId.IsValid())
        {
            Physics::RigidBodyRequestBus::Event(ballEntityId, &Physics::RigidBodyRequestBus::Events::ApplyLinearImpulse, GetForce());
        }
    }

    void NetworkBallMovementComponentController::UpdateForce(const NetworkBallMovementComponentNetworkInput& playerInput)
    {
        SetForce(playerInput.m_translationDiff * playerInput.m_power * 77);
    }

    void NetworkBallMovementComponentController::OnPressed([[maybe_unused]] float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }

        if (!(*inputId == ImpulseEventId || *inputId == Impulse2EventId))
        {
            return;
        }

        m_impulseReleased = false;
        m_impulse2Released = false;
        m_power = 0;

        AZ::TickBus::Handler::BusConnect();
    }

    void NetworkBallMovementComponentController::OnReleased([[maybe_unused]] float value)
    {
        const StartingPointInput::InputEventNotificationId* inputId = StartingPointInput::InputEventNotificationBus::GetCurrentBusId();

        if (inputId == nullptr)
        {
            return;
        }

        if (!(*inputId == ImpulseEventId || *inputId == Impulse2EventId))
        {
            return;
        }

        AZ::TickBus::Handler::BusDisconnect();

        if (*inputId == ImpulseEventId)
        {
            m_impulseReleased = true;
        }
        else if (*inputId == Impulse2EventId)
        {
            m_impulse2Released = true;
        }
    }

    void NetworkBallMovementComponentController::OnHeld([[maybe_unused]] float value)
    {
    }
} // namespace MultiplayerSample
