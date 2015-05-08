-- This script implements grenade behaviour for both
-- Hand Grenades as well as Assault Rifle Grenades.
local Grenade = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

local Trafo = Grenade:GetEntity():GetTransform()
local Model = Grenade:GetEntity():GetComponent("Model")
local Light = Grenade:GetEntity():GetComponent("PointLight")
local PlPhy = Grenade:GetEntity():GetComponent("PlayerPhysics")

Grenade.EVENT_TYPE_EXPLODED = 1
Grenade.Timer               = 0.0

Grenade:InitEventTypes(1)

-- TODO: Call InitClientApprox() in some client-init (e.g. OnClientInit()) only?
Trafo:InitClientApprox("Origin")


function Grenade:Think(FrameTime)
    -- Move the grenade if the timer is still ticking.
    if Model:get("Show") then
        PlPhy:MoveHuman(FrameTime, 0, 0, 0, 0, 0, 0, false)
    end

    -- Let the detonation timer tick.
    local OldTimer = self.Timer
    self.Timer = self.Timer + FrameTime

    -- If the timer passes 3.0 seconds, let the grenade explode.
    if OldTimer < 3.0 and self.Timer >= 3.0 then
        Model:set("Show", false)
        Light:set("On", true)

        -- Inflict damage to all nearby entities.
        self:DamageAll(100.0, 40.0, 200.0)

        self:PostEvent(self.EVENT_TYPE_EXPLODED)
    end

    if self.Timer >= 6.0 then
        Light:set("On", false)

        -- Remove this entity from the game world (it exploded about 3 seconds ago).
        local Entity = self:GetEntity()
        local Parent = Entity:GetParent()

        Parent:RemoveChild(Entity)
    end
end


-- This method is called automatically on the client whenever an event arrives.
function Grenade:ProcessEvent(EventType)  -- (EventType, NumEvents)
    if EventType == self.EVENT_TYPE_EXPLODED then
        -- Main explosion particle system.
        local PaSys1 = self:GetEntity():GetComponent("ParticleSystemOld", 0)
        PaSys1:EmitParticle()

        -- Explosion "sparkles" particle system.
        local PaSys2 = self:GetEntity():GetComponent("ParticleSystemOld", 1)
        for i = 1, 20 do
            PaSys2:EmitParticle()
        end

        self:GetEntity():GetComponent("Sound"):Play()
    end
end


local clTime = 0.0

function Light:ClientEffect(t)
    if not self:get("On") then
        clTime = 0.0
        return false
    end

    clTime = clTime + t
    local Duration = Grenade.LightDuration or 5.0

    if clTime >= Duration then
        return true, 0, 0, 0, 0
    end

    local Amount = 1.0 - clTime/Duration

    return true, Amount, Amount*Amount, Amount*Amount*Amount, 400.0
end
