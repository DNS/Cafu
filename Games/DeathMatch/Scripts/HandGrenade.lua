local HandGrenade = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

local Trafo = HandGrenade:GetEntity():GetTransform()
local Model = HandGrenade:GetEntity():GetComponent("Model")
local Light = HandGrenade:GetEntity():GetComponent("PointLight")
local PlPhy = HandGrenade:GetEntity():GetComponent("PlayerPhysics")

HandGrenade.EVENT_TYPE_EXPLODED = 1
HandGrenade.Timer               = 0.0

HandGrenade:InitEventTypes(1)


-- There is no need to call InitClientApprox() here, because HandGrenades have
-- a PlayerPhysics component that sets interpolation for the origin as well.
if false then
    -- TODO: Call InitClientApprox() in some client-init (e.g. OnClientInit()) only?
    Trafo:InitClientApprox("Origin")
end


function HandGrenade:Think(FrameTime)
    -- Let the detonation timer tick.
    local OldTimer = self.Timer
    self.Timer = self.Timer + FrameTime

    -- The PlayerPhysics component independently moves the grenade while the
    -- timer is ticking. If it passes 3.0 seconds, let the grenade explode.
    if OldTimer < 3.0 and self.Timer >= 3.0 then
        Model:set("Show", false)
        Light:set("On", true)

        -- Disable the PlayerPhysics component, it is no longer needed.
        self:GetEntity():RemoveComponent(PlPhy)

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
function HandGrenade:ProcessEvent(EventType)  -- (EventType, NumEvents)
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
local Duration = 0.5

function Light:ClientEffect(t)
    if not self:get("On") then
        clTime = 0.0
        return false
    end

    clTime = clTime + t

    if clTime >= Duration then
        return true, 0, 0, 0, 0
    end

    local Amount = 1.0 - clTime/Duration

    return true, Amount, Amount*Amount, Amount*Amount*Amount, 400.0
end