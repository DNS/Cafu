local Weapon = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

Weapon.Trafo                = Weapon:GetEntity():GetTransform()
Weapon.Model                = Weapon:GetEntity():GetComponent("Model")
Weapon.EVENT_TYPE_PICKED_UP = 1
Weapon.EVENT_TYPE_RESPAWN   = 2
Weapon.TimeLeftNotActive    = 0.0

Weapon:InitEventTypes(2)

-- This is needed because client effects are applied to the
-- transform's origin in Weapon:OnClientFrame().
Weapon.Trafo:InitClientApprox("Origin")

-- Drop weapons to the ground. It doesn't look good when they hover in the air.
do
    local Origin = { Weapon.Trafo:get("Origin") }
    local Result = world:TraceRay(Origin, { 0, 0, -1000.0 })

    Origin[3] = Origin[3] - 1000.0 * Result.Fraction

    Weapon.Trafo:set("Origin", Origin)
end

-- Add a trigger volume to our weapon, or else the OnTrigger() callback below will never be called.
-- Alternatively, it would also be possible to add trigger brushes to the entity in the Map Editor;
-- in fact, the purpose of adding the needed trigger volume here is to free both the mapper and the
-- weapon prefab from explicitly having themselves to concern with it.
do
    local Trigger = world:new("ComponentCollisionModelT")

    Weapon:GetEntity():AddComponent(Trigger)

    Trigger:set("IgnoreOrient", true)
    -- Trigger:SetBoundingBox(Weapon.Model:GetBoundingBox(), "Textures/meta/trigger")   -- A possible future improvement.
    Trigger:SetBoundingBox(-8, -8, 0, 8, 8, 8, "Textures/meta/trigger")
end


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


-- This method is called automatically whenever `Ent` finds itself in this weapon's trigger volume.
function Weapon:OnTrigger(Ent)
    -- print("This weapon (", self, ") is touched by", Ent)
    -- print("Weapon entity:", self:GetEntity())

    -- If we are touched when not being "active", ignore the touch.
    if not self:IsActive() then return end

    local EntScript = Ent:GetComponent("Script")

    -- If the entity that triggered us does not have a Script component,
    -- or the entity's Script does not have the PickUpItem() callback defined,
    -- it is not ready to pick up an item or weapon.
    if not EntScript then return end
    if not EntScript.PickUpItem then return end

    local s = self.Model:get("Name")
    s = string.gsub(s, ".*/", "")
    s = string.gsub(s, "_w%.cmdl$", "")
    s = string.gsub(s, "%.cmdl$", "")

    if EntScript:PickUpItem(self, s) then
        -- Finally retire for a while, that is, deactivate this weapon for a few seconds.
        -- While the weapon is deactivated, it is invisible and cannot be picked up.
        self.TimeLeftNotActive = 5.0
        self.Model:set("Show", false)
        self:PostEvent(self.EVENT_TYPE_PICKED_UP)
    end
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


local arc = 0.0
local frq = 2.0 + math.random()

function Weapon:OnClientFrame(FrameTime)
    arc = (arc + frq * FrameTime) % (2.0 * math.pi)

    local Origin = { self.Trafo:get("Origin") }
    local ofs = math.sin(arc)

    Origin[3] = Origin[3] + 1.5 * (1.0 + ofs)

    self.Trafo:set("Origin", Origin)
end
