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
local Logger = require('Scripts.TestHarness.Logger').Create("SanityTest")
local Test = require('Scripts.TestHarness.Helpers.Test')

local SanityTest =
{
    Properties =
    {
        Pass = {default=true, description="pass the sanity test (fail if unchecked)"},
        Delay = {default=0.0, description="(seconds) delay completion of sanity test"},
    },

    testBus = Test.SafeBus(TestCaseRequestBus),
    tickBus = Test.SafeBus(TickBus),

    delay = 0.0,
}

function SanityTest:OnActivate()
    Test.Activate(self)
end

function SanityTest:OnDeactivate()
    Test.Deactivate(self)
end

---------------------
-- TestCaseRequestBus
---------------------

function SanityTest:Setup()
    self.delay = self.Properties.Delay
    Test.SetupCompleted(self)
end

function SanityTest:TearDown()
    Test.EBusDisconnect(self.tickBus)
end

function SanityTest:Execute()
    Test.EBusConnect(self, self.tickBus, 0)
end

----------
-- TickBus
----------

function SanityTest:OnTick(deltaTime, timePoint)
    self.delay = self.delay - deltaTime
    if self.delay < 0.0 then
        Test.EBusDisconnect(self.tickBus)
        Test.ExpectTrue(self, self.Properties.Pass)
        Test.Completed(self)
    end
end

return SanityTest