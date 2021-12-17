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
local Logger = require('Scripts.TestHarness.Logger').Create("StopBallTest")
local Test = require('Scripts.TestHarness.Helpers.Test')

local StopBallTest =
{
    Properties =
    {
        Player = {default=EntityId(), description="Player character"},
        PhysicsEntity = {default=EntityId(), description="Physics entity to shoot out"},
        BallEntity = {default=EntityId(), description="Ball entity to stop"},
    },

    testBus = Test.SafeBus(TestCaseRequestBus),
    tickBus = Test.SafeBus(TickBus),
}

function StopBallTest:OnActivate()
    Test.Activate(self)
end

function StopBallTest:OnDeactivate()
    Test.Deactivate(self)
end

function StopBallTest:Setup()
    -- Moving player to the area of the test so we can watch
    local testLocation = TransformBus.Event.GetWorldTranslation(self.entityId)
    TransformBus.Event.SetWorldTranslation(self.Properties.Player, testLocation)
    Test.SetupCompleted(self)
end

function StopBallTest:TearDown()
    Test.EBusDisconnect(self.tickBus)
end

function StopBallTest:Execute()
    Test.EBusConnect(self, self.tickBus, 0)
    PhysicsComponentRequestBus.Event.SetVelocity(self.Properties.PhysicsEntity, Vector3(0,10,0))
end

function StopBallTest:OnTick(deltaTime, timePoint)
    local test = PhysicsComponentRequestBus.Event.GetVelocity(self.Properties.BallEntity)
    if test.x < 0.3 and test.y < 0.3 and test.z < 0.3 then
        Test.EBusDisconnect(self.tickBus)
        Test.Completed(self)
    end
end

return StopBallTest