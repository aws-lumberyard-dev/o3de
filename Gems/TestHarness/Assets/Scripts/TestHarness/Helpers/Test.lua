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
local Test = {}

-- return an entity's name given its entityId
-- @param entityId the entityId of the entity we want the name of
-- @return the name of the entity
function Test.GetEntityName(entityId)
    local name = GameEntityContextRequestBus.Broadcast.GetEntityName(entityId)
    return name
end

-- verifies if the entityId is in a given array
-- @param entityId the Id we're looking for
-- @param array the array we're looking in
-- @return true if in the array, false if not
function Test.IsEntityIdInArray(entityId, array)
    local n = #array
    for i=1, #array do
        if entityId == array[i] then
            return true
        end
    end
    return false
end

-- wrapper for getting an EBus interface
-- @param ebus the ebus we want to get an interface for
-- @return the EBus interface
function Test.SafeBus(ebus)
    return {
        Connect = function(self, ...)
            self.handler = ebus.Connect(...)
        end,

        Disconnect = function(self)
            if self.handler then
                self.handler:Disconnect()
                self.handler = nil
            end
        end,

        handle = nil
    }
end

-- connects the test to the TestCaseRequestBus
-- should be called in the test's OnActivate function
-- @param target the target test we want connected (normally 'self')
function Test.Activate(target)
    if target.entityId == nil then
        error("Invalid entityId given to Activate")
    elseif target.testBus == nil then
        error("Invalid bus given to Activate")
    else
        target.testBus:Connect(target, target.entityId)
    end
end

-- disconnects the test to the TestCaseRequestBus
-- should be called in the test's OnDeactivate function
-- @param target the target test we want disconnected (normally 'self')
function Test.Deactivate(target)
    if target.entityId == nil then
        error("Invalid entityId given to Deactivate")
    elseif target.testBus == nil then
        error("Invalid bus given to Deactivate")
    else
        target.testBus:Disconnect()
    end
end

-- general bus connect function that can be used for multiple different buses
-- @param target the target we want connected (normally 'self')
-- @param bus the bus we want to connect to
-- @param param the parameter the connection bus connection needs
function Test.EBusConnect(target, bus, connectParam)
    if target.entityId == nil then
        error("Invalid entityId given to EBusConnect")
    elseif bus == nil then
        error("Invalid bus given to EBusConnect")
    else
        bus:Connect(target, connectParam)
    end
end

-- general bus disconnect function that can be used for multiple different buses
-- @param bus the bus we want to disconnect from
function Test.EBusDisconnect(bus)
    if bus == nil then
        error("Invalid target given to EBusDisconnect")
    else
        bus:Disconnect()
    end
end

-- marks this test as finished setting up
-- @param target the test that is finished setting up (normally 'self')
function Test.SetupCompleted(target)
    if target.entityId == nil then
        error("Invalid target given to SetupCompleted")
    else
        TestCaseEventBus.Event.SetupCompleted(target.entityId)
    end
end

-- sends truth value to internal test runner to be evaluated
-- @param target the test we are running (normally 'self')
-- @param shouldPass the truth value we are sending to the internal test runner
function Test.ExpectTrue(target, shouldPass)
    if target.entityId == nil then
        error("Invalid entityId given to ExpectTrue")
    elseif shouldPass == nil then
        error("Invalid truth statement given to ExpectTrue")
    else
        TestCaseEventBus.Event.ExpectTrue(target.entityId, shouldPass)
    end
end

-- Marks this test as finished executing
-- @param target the test that has finished executing (normally 'self')
function Test.Completed(target)
    if target.entityId == nil then
        error("Invalid target given to Completed")
    else
        TestCaseEventBus.Event.Completed(target.entityId)
    end  
end

return Test