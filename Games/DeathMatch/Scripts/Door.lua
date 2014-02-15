local Door = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.


function Door:OnTrigger(Ent)
    -- Don't re-enter this code while we're still busy.
    if self.IsBusy then return end
    self.IsBusy = true

    Console.Print("An entity is inside the door's trigger volume!\n")
    -- print(Ent, "has triggered door", self, ",", self:GetEntity())

    -- Wait 3 secs, then become available again for the next trigger action.
    coroutine.yield(3)
    self.IsBusy = false
end
