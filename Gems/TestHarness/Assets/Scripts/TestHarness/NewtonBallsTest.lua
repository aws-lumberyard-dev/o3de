--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
local Logger = require('Scripts.TestHarness.Logger').Create("NewtonBallsTest")
local Test = require('Scripts.TestHarness.Helpers.Test')

local NewtonBallsTest =
{
    Properties =
    {
        Player = {default=EntityId(), description="Player character"},
        StartBallEntity = {default=EntityId(), description="The starting ball in the train"},
        EndBallEntity = {default=EntityId(), description="The starting ball in the train"},
        TriggerEntity = {default=EntityId(), description="Trigger area to monitor"},
    },

    testBus = Test.SafeBus(TestCaseRequestBus),
    triggerAreaNotificationBus = Test.SafeBus(TriggerAreaNotificationBus),
}

function NewtonBallsTest:OnActivate()
    Test.Activate(self)
end

function NewtonBallsTest:OnDeactivate()
    Test.Deactivate(self)
end

function NewtonBallsTest:Setup()
    -- Moving player to the area of the test so we can watch
    Test.EBusConnect(self, self.triggerAreaNotificationBus, self.Properties.TriggerEntity)
    local testLocation = TransformBus.Event.GetWorldTranslation(self.entityId)
    TransformBus.Event.SetWorldTranslation(self.Properties.Player, testLocation)
    Test.SetupCompleted(self)
end

function NewtonBallsTest:TearDown()
    Test.EBusDisconnect(self.triggerAreaNotificationBus)
end

function NewtonBallsTest:Execute()
    PhysicsComponentRequestBus.Event.SetVelocity(self.Properties.StartBallEntity, Vector3(-50, 0, 0))
end

function NewtonBallsTest:OnTriggerAreaEntered(enteringEntity)
    local correctEntity = enteringEntity == self.Properties.EndBallEntity
    if correctEntity then
        Test.EBusDisconnect(self.triggerAreaNotificationBus)
        Test.Completed(self)
    end
end

return NewtonBallsTest