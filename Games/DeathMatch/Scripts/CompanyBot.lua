local CompanyBot   = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.
local Entity       = CompanyBot:GetEntity()
local Trafo        = Entity:GetTransform()
local Model        = Entity:GetComponent("Model")
local PlPhysics    = Entity:GetComponent("PlayerPhysics")
local LanternEnt   = Entity:GetChildren()[1]
local LanternTrafo = LanternEnt:GetTransform()
local LanternLight = LanternEnt:GetComponent("PointLight")

CompanyBot.Health = 80

-- TODO: Should we also have a *Physics* component (of kinematic type)?

Trafo:InitClientApprox("Origin")
-- Trafo:InitClientApprox("Orientation")  -- We manually update the orientation via LookAt() in Think(), but this looks better uninterpolated.

-- This is needed because client effects are applied to the lantern's origin,
-- color and radius in LanternTrafo:OnClientFrame() and LanternLight:OnClientFrame().
LanternTrafo:InitClientApprox("Origin")
LanternLight:InitClientApprox("Color")
LanternLight:InitClientApprox("Radius")


-- Drop CompanyBots to the ground. It doesn't look good when they hover in the air.
-- Note that this must be done *before* the collision model is added below, or else our
-- traces would start in solid, or the collision model explicitly had to be ignored.
do
    -- TODO: The original (old) C++ code did this by tracing a *bounding-box* first up,
    --       then down, making sure the required volume is really clear of obstacles.
    --       The `world` API should be augmented so that this can be implemented again.
    -- TODO: If this is *omitted*, and at the start of a level a CompanyBot falls from
    --       a height of e.g. 200 units, it *seems* to bounce some way into the ground,
    --       then back up to the proper level. Refute or confirm, debug if necessary.
    local Origin = { Trafo:get("Origin") }
    local Result = world:TraceRay(Origin, { 0, 0, -1000.0 })

    Origin[3] = Origin[3] - 1000.0 * Result.Fraction + 36.0

    Trafo:set("Origin", Origin)
end


-- Add a collision model to our CompanyBot, so that it participates in collision detection.
-- Alternatively, it would also be possible to add a collision model to the entity in the Map Editor;
-- in fact, the purpose of adding the needed collision model here is to free both the mapper and the
-- CompanyBot prefab from explicitly having themselves to concern with it.
do
    local CollMdl = world:new("ComponentCollisionModelT")

    Entity:AddComponent(CollMdl)

    CollMdl:set("IgnoreOrient", true)
    -- CollMdl:SetBoundingBox(Model:GetBoundingBox(), "Textures/meta/collisionmodel")   -- A possible future improvement.
    CollMdl:SetBoundingBox(-12, -12, -36, 12, 12, 36, "Textures/meta/collisionmodel")
end


function CompanyBot:TakeDamage(OtherEnt, Amount, ImpDirX, ImpDirY, ImpDirZ)
    if self.Health <= 0 then
        -- Dead CompanyBots can take no further damage.
        return
    end

    -- Nudge the entity into the direction of ImpDir.
    local vx, vy, vz = PlPhysics:get("Velocity")

    vx = vx + 20.0 * Amount * ImpDirX
    vy = vy + 20.0 * Amount * ImpDirY

    PlPhysics:set("Velocity", vx, vy, vz)

    -- Reduce the entity's health.
    self.Health = self.Health - Amount
    Console.Print("Something damaged the CompanyBot, new health is " .. tostring(self.Health) .. "\n")

    if (self.Health <= 0) then
        self.Health = 0

        local fx, fy, fz = Trafo:GetAxisX()     -- "forward" axis
        local lx, ly, lz = Trafo:GetAxisY()     -- "left" axis

        local d1 = fx*ImpDirX + fy*ImpDirY + fz*ImpDirZ
        local d2 = lx*ImpDirX + ly*ImpDirY + lz*ImpDirZ

        if d1 > 0.7 then
            Model:set("Animation", 21)                      -- die forwards
        elseif d1 > 0.0 then
            Model:set("Animation", d2 > 0 and 23 or 22)     -- die spin, headshot
        elseif d1 > -0.7 then
            Model:set("Animation", d2 > 0 and 18 or 24)     -- die simple, gutshot
        else
            Model:set("Animation", d2 > 0 and 20 or 19)     -- die backwards, die backwards1
        end

        -- Now that it is dead, clear the CompanyBot's collision model.
        Entity:GetComponent("CollisionModel"):set("Name", "")

        -- TODO: Count the frag at the "creator" entity (OtherEnt, or a parent thereof).
        --       OtherEnt:AddFrag()
    end
end


function CompanyBot:Think(FrameTime)
    if self.Health <= 0 then
        -- As the PlayerPhysics component has been configured above already,
        -- there is nothing else to do.
        return
    end

    -- TODO: We should find them by some player-specific component,
    --       ideally also checking if they're still alive.
    local HumanPlayer = Entity:GetRoot():FindByName("Player_1")
    if not HumanPlayer then return end

    local Dist = Vector3T(HumanPlayer:GetTransform():get("Origin")) - Vector3T(Trafo:get("Origin"))
    Dist[3] = 0

    local DistLen = math.sqrt(Dist[1]*Dist[1] + Dist[2]*Dist[2] + Dist[3]*Dist[3])
    local WishVel = Vector3T()

    if DistLen > 400.0 then
        -- Have it look towards HumanPlayer.
        local px, py, pz = HumanPlayer:GetTransform():get("Origin")
        Trafo:LookAt(px, py, pz, 0, true)

        -- Set a speed towards HumanPlayer.
        WishVel = Dist / DistLen * 280.0
    end

    PlPhysics:MoveHuman(FrameTime, WishVel[1], WishVel[2], WishVel[3], 0, 0, 0, false)

    local vx, vy, vz = PlPhysics:get("Velocity")
    local Speed      = math.sqrt(vx*vx + vy*vy)

    Model:set("Animation", Speed < 40.0 and 1 or 3)
end


function Model:OnAnimationChange(AnimNr)
    if (AnimNr < 18) or (AnimNr > 24) then
        -- The CompanyBot is "alive", so blend animation sequences in 0.3 seconds,
        -- and force their playback in a loop.
        return 0.3, true
    end

    -- TODO: It would be nice if we also had the number of frame and FPS of the animation sequence,
    -- and could then use   math.min(0.2, (NumFrames - 1) * FPS * 0.5)   as blend time.
    return 0.2, false
end


local OscilTime = 0.0

function LanternTrafo:OnClientFrame(FrameTime)
    OscilTime = (OscilTime + FrameTime) % 4.0

    local lx = 20.0
    local ly =  3.0 * math.cos(OscilTime / 2.0 * math.pi)   -- OscilTime/4 * 2pi
    local lz = 12.0 + 0.666 * ly

    self:set("Origin", lx, ly, lz)
end


local LightTime = 0.0

function LanternLight:OnClientFrame(t)
    LightTime = LightTime + t
    if LightTime >= 6.0 then LightTime = LightTime - 4.0 end

    local r = 0.0
    local g = 0.0

    if LightTime < 2.0 then
        -- 0.0 <= LightTime < 2.0
        r = 1.0 - 0.5 * (1.0 + math.cos(LightTime / 2.0 * math.pi))
        g = r * 0.9
    else
        -- 2.0 <= LightTime < 6.0
        r = 1.0
        g = 0.5 * (1.0 + math.cos((LightTime - 2.0) / 2.0 * math.pi))
        g = g * 0.8 + 0.1
    end

    self:set("Color", r, g, 0.0)
    self:set("Radius", 200.0)
end
