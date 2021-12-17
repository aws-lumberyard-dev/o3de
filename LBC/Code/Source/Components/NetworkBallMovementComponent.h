/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/NetworkBallMovementComponent.AutoComponent.h>
#include <StartingPointInput/InputEventNotificationBus.h>

#include <AzCore/Component/TickBus.h>

namespace LBC
{
    // Input Event Ids for Player Controls
    const StartingPointInput::InputEventNotificationId ImpulseEventId("impulse");
    const StartingPointInput::InputEventNotificationId Impulse2EventId("impulse2");

    class NetworkBallMovementComponentController
        : public NetworkBallMovementComponentControllerBase
        , private StartingPointInput::InputEventNotificationBus::MultiHandler
        , public AZ::TickBus::Handler
    {
    public:
        NetworkBallMovementComponentController(NetworkBallMovementComponent& parent);

        //! NetworkBallMovementComponentControllerBase
        //! @{
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        void CreateInput(Multiplayer::NetworkInput& input, float deltaTime) override;
        void ProcessInput(Multiplayer::NetworkInput& input, float deltaTime) override;
        //! @}

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
    
    private:
        friend class NetworkAiComponentController;

        void UpdateForce(const NetworkBallMovementComponentNetworkInput& playerInput);

        //! AZ::InputEventNotificationBus interface
        //! @{
        void OnPressed(float value) override;
        void OnReleased(float value) override;
        void OnHeld(float value) override;
        //! @}

        float m_power = 0;

        bool m_impulseReleased = false;
        bool m_impulse2Released = false;
    };
}
