local Weapon = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

Weapon.Model                = Weapon:GetEntity():GetComponent("Model")
Weapon.EVENT_TYPE_PICKED_UP = 1
Weapon.EVENT_TYPE_RESPAWN   = 2
Weapon.TimeLeftNotActive    = 0.0

Weapon:InitEventTypes(2)


-- Returns whether the weapon is currently active (visible and can be picked up).
function Weapon:IsActive()
    return self.TimeLeftNotActive <= 0.0
end


function Weapon:Think(FrameTime)
    if self:IsActive() then
        -- Already active?
        -- No need to do anything while we're waiting to be touched.
        return
    end

    self.TimeLeftNotActive = self.TimeLeftNotActive - FrameTime

    if self:IsActive() then
        -- Just got re-activated?
        self.Model:set("Show", true)
        self:PostEvent(self.EVENT_TYPE_RESPAWN)
    end
end


function Weapon:NotifyTouchedBy(Ent)
    -- print("This weapon (", self, ") is touched by", Ent)
    -- print("Weapon entity:", self:GetEntity())

    -- If we are touched when not being "active", ignore the touch.
    if not self:IsActive() then return end

    -- If we are touched by anything else than a human player, ignore the touch.
    -- Would be interesting to also allow touchs by bots, though.
    -- TODO: if (Entity->GetType()!=&EntHumanPlayerT::TypeInfo) return;

    -- Give this weapon to the entity.
    -- TODO: if (!GameImplT::GetInstance().GetCarriedWeapon(WEAPON_SLOT_357)->ServerSide_PickedUpByEntity(dynamic_cast<EntHumanPlayerT*>(Entity))) return;

    -- And finally retire for a while, that is, deactivate this weapon for a few seconds.
    -- While the weapon is deactivated, it is invisible and cannot be picked up.
    self.TimeLeftNotActive = 5.0
    self.Model:set("Show", false)
    self:PostEvent(self.EVENT_TYPE_PICKED_UP)

    -- The following is a work-around for something like:
    --[[
    local Inv = Ent:GetComponent("Inventory")
    if Inv then
        local s = .......
        Inv:AddWeapon(s)
    end
    ]]--
    local s = self.Model:get("Name")
    s = string.gsub(s, ".*/", "")
    s = string.gsub(s, "[_.].*", "")

    print("Picked up weapon:", s)
    return s
end


-- This method is called automatically on the client whenever an event arrives.
function Weapon:ProcessEvent(EventType)   -- (EventType, NumEvents)
    --[[
    if EventType == self.EVENT_TYPE_PICKED_UP then
        Weapon:GetEntity():GetComponent("Sound", 0):Play()
    elseif EventType == self.EVENT_TYPE_RESPAWN then
        Weapon:GetEntity():GetComponent("Sound", 1):Play()
    end
    --]]

    Weapon:GetEntity():GetComponent("Sound", EventType - 1):Play()
end
