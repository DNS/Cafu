dofile("Games/DeathMatch/Scripts/Vector3.lua")

local Mover = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

Mover.Trafo   = Mover:GetEntity():GetTransform()
Mover.CollMdl = Mover:GetEntity():GetComponent("CollisionModel")

Mover.LinPushMove_TimeLeft = 0.0

-- TODO: Call InitClientApprox() in some client-init (e.g. OnClientInit()) only?
Mover.Trafo:InitClientApprox("Origin")
-- Mover.Trafo:InitClientApprox("Orientation")


Mover.isMoving = false
Mover.isHome   = true
Mover.Origin   = { Mover.Trafo:get("Origin") }


function Mover:OnActivate(OtherEnt)
    Console.Print("Hello from " .. self:GetEntity():GetBasics():get("Name") .. "'s OnActivate(), called by " .. OtherEnt:GetBasics():get("Name") .. "!\n")

    if self.isMoving then
        -- If we're moving already, do nothing else until we're done.
        return
    end

    self.isMoving = true
    local variant = 2

    if variant == 1 then
        self:LinearPushMove(self.Origin[1], self.Origin[2], self.Origin[3] + 80.0, 1.5)
        coroutine.yield(1.5 + 1.0)

        self:LinearPushMove(self.Origin[1], self.Origin[2], self.Origin[3], 1.5)
        coroutine.yield(1.5)
    else
        if self.isHome then
            self:LinearPushMove(self.Origin[1], self.Origin[2], self.Origin[3] + 120.0, 1.5)
        else
            self:LinearPushMove(self.Origin[1], self.Origin[2], self.Origin[3], 1.5)
        end

        coroutine.yield(1.5)
        self.isHome = not self.isHome
    end

    self.isMoving = false
end


function Mover:Think(FrameTime)
    if self.LinPushMove_TimeLeft > 0.0 then
        local FrameTime = math.min(FrameTime, self.LinPushMove_TimeLeft)

        self.LinPushMove_TimeLeft = self.LinPushMove_TimeLeft - FrameTime


        -- Drag all entities touched by us further along our path as well.
--[[
        local Ray = (self.LinPushMove_Dest - self.LinPushMove_Source) * (FrameTime / self.LinPushMove_TimeTotal)

        local Contacts = CollMdl:GetContacts(self.Trafo:get("Origin"), Ray)

        for Ent in pairs(Contacts) do
        end
--]]


        -- Move this entity to its new place along the path as well.
        local frac = self.LinPushMove_TimeLeft / self.LinPushMove_TimeTotal

        self.Trafo:set("Origin",
            self.LinPushMove_Source*frac + self.LinPushMove_Dest*(1.0 - frac))
    end
end


function Mover:LinearPushMove(dx, dy, dz, t)
    self.LinPushMove_Source    = Vector3T(self:GetEntity():GetTransform():get("Origin"))
    self.LinPushMove_Dest      = Vector3T(dx, dy, dz)
    self.LinPushMove_TimeTotal = t
    self.LinPushMove_TimeLeft  = t
end
