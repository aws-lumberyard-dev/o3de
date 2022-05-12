/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/IMultiplayerSpawner.h>
#include <gmock/gmock.h>

// The following is a mock of IMultiplayerSpawner used to test join/leave in MultiplayerSystemComponent

class IMultiplayerSpawnerMock : public Multiplayer::IMultiplayerSpawner
    {
    public:        
        IMultiplayerSpawnerMock()
        {
            ;
        }

        ~IMultiplayerSpawnerMock() override{};

        void OnPlayerJoin(
            [[maybe_unused]] uint64_t userId,
            [[maybe_unused]] const Multiplayer::MultiplayerAgentDatum& agentDatum,
            [[maybe_unused]] AzFramework::EntitySpawnCallback playerEntityPreInsertionCallback) override
        {
            ++m_playerCount;
        }

        void OnPlayerLeave(
            [[maybe_unused]] Multiplayer::ConstNetworkEntityHandle entityHandle,
            [[maybe_unused]] const Multiplayer::ReplicationSet& replicationSet,
            [[maybe_unused]] AzNetworking::DisconnectReason reason) override
        {
            --m_playerCount;
        }
    
        int32_t m_playerCount = 0;
    };

