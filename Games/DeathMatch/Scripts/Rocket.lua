local Rocket = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

local Trafo = Rocket:GetEntity():GetTransform()
local Model = Rocket:GetEntity():GetComponent("Model")
local Light = Rocket:GetEntity():GetComponent("PointLight")

Rocket.EVENT_TYPE_EXPLODED = 1
Rocket.TimeSinceExploded   = 0.0

Rocket:InitEventTypes(1)

Trafo:InitClientApprox("Origin")

-- This is needed because client effects are applied to the
-- light's color and radius in Light:OnClientFrame().
Light:InitClientApprox("Color")
Light:InitClientApprox("Radius")


function Rocket:Think(FrameTime)
    if self.TimeSinceExploded == 0.0 then
        local Origin     = Vector3T(Trafo:get("Origin"))
        local FlightDist = Vector3T(Trafo:GetAxisX()) * 560.0 * FrameTime
        local Result     = world:Phys_TraceBB({ -4, -4, -4, 4, 4, 4 }, Origin, FlightDist)

        Trafo:set("Origin", Origin + FlightDist*Result.closestHitFraction)

        if Result.hasHit then
            self.TimeSinceExploded = self.TimeSinceExploded + FrameTime

            -- This is a hack to move the particle effect and the light source of the explosion
            -- a bit back from the wall that we just hit...
            Trafo:set("Origin", Origin - Vector3T(Trafo:GetAxisX()) * 16.0)

            Model:set("Show", false)
            Light:set("ShadowType", 1)

            -- Inflict damage to all nearby entities.
            self:DamageAll(100.0, 40.0, 200.0)

            self:PostEvent(self.EVENT_TYPE_EXPLODED)
        end
    else
        self.TimeSinceExploded = self.TimeSinceExploded + FrameTime

        -- Wait 3 seconds after the explosion, so that the explosion event can
        -- travel to the clients. Finally remove this entity from the game world.
        if self.TimeSinceExploded >= 3.0 then
            Light:set("On", false)

            -- Remove this entity from the game world (it exploded about 3 seconds ago).
            local Entity = self:GetEntity()
            local Parent = Entity:GetParent()

            Parent:RemoveChild(Entity)
        end
    end
end


-- This method is called automatically on the client whenever an event arrives.
function Rocket:ProcessEvent(EventType)  -- (EventType, NumEvents)
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
local Duration = 3.0

function Light:OnClientFrame(t)
    -- Use the model's "Show" variable in order to learn if we're still under way or exploded already
    -- (alternatively, we could use self:get("ShadowType") as well).
    if Model:get("Show") then
        -- Use whatever values the light source actually has.
        return
    end

    clTime = clTime + t

    if clTime >= Duration then
        self:set("Color", 0, 0, 0)
        self:set("Radius", 0)
        return
    end

    local Amount = 1.0 - clTime/Duration

    self:set("Color", Amount, Amount*Amount, 0.0)
    self:set("Radius", 1000.0)
end
