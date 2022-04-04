

local SomeClass = 
{
    Properties =
    {
    }
}

function SomeClass:OnActivate()
    self.tickBusHandler = TickBus.CreateHandler(self)
    self.tickBusHandler:Connect()

    Debug.Log(package.cpath)
end


function SomeClass:OnTick(deltaTime, timePoint)
    Debug.Log("Test Success = " .. tostring(deltaTime))
    
end


return SomeClass