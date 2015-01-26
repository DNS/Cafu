dofile("Games/DeathMatch/Scripts/Vector3.lua")

local Script = ...      -- Retrieve the ComponentScriptT instance that is responsible for this script.
local Mover  = Script:GetEntity():GetComponent("Mover")
local Trafo  = Script:GetEntity():GetTransform()

local STATE_AT_HOME        = 0
local STATE_MOVING_TO_DEST = 1
local STATE_AT_DEST        = 2
local STATE_MOVING_BACK    = 3

Script.State    = STATE_AT_HOME     -- Initial state is in the home rest position.
Script.Progress = 0.0               -- How far along the way to the "dest" position?
Script.Duration = 3.0               -- How long will a move take?  This should be configurable!

Script.ORIGIN_HOME = Vector3T(Trafo:get("Origin"))
Script.ORIGIN_DEST = Script.ORIGIN_HOME + Vector3T(0.0, 0.0, 80.0)

-- TODO: Call InitClientApprox() in some client-init (e.g. OnClientInit()) only?
Trafo:InitClientApprox("Origin")
-- Trafo:InitClientApprox("Orientation")


function Script:OnActivate(OtherEnt)
    Console.Print("Hello from " .. self:GetEntity():GetBasics():get("Name") .. "'s OnActivate(), called by " .. OtherEnt:GetBasics():get("Name") .. "!\n")

    if self.State ~= STATE_AT_HOME then
        -- If we're moving already, do nothing else until we're done.
        return
    end

    self.State    = STATE_MOVING_TO_DEST
    self.Progress = 0.0
end


function Script:GetMove(PartNr, FrameTime)
    if self.State == STATE_AT_HOME or
       self.State == STATE_AT_DEST then
        return
    end

    if PartNr ~= 0 then
        return
    end

    local p

    if self.State == STATE_MOVING_TO_DEST then
        p = math.min(self.Progress + FrameTime / self.Duration, 1.0)
    elseif self.State == STATE_MOVING_BACK then
        p = math.max(self.Progress - FrameTime / self.Duration, 0.0)
    end

    local NewPos = self.ORIGIN_HOME * (1.0 - p) + self.ORIGIN_DEST * p
    local Delta  = NewPos - Vector3T(Trafo:get("Origin"))

    return self:GetEntity():GetID(), Delta[1], Delta[2], Delta[3]
end


function Script:Think(FrameTime)
    if self.State == STATE_MOVING_TO_DEST then
        if Mover:HandleMove(FrameTime) then
            self.Progress = self.Progress + FrameTime / self.Duration

            if self.Progress >= 1.0 then
                self.State    = STATE_AT_DEST
                self.Progress = 1.0

                -- Auto-activation: Wait a moment at "dest", then move back.
                coroutine.yield(2.5)

                self.State    = STATE_MOVING_BACK
                self.Progress = 1.0
            end
        end
    elseif self.State == STATE_MOVING_BACK then
        if Mover:HandleMove(FrameTime) then
            self.Progress = self.Progress - FrameTime / self.Duration

            if self.Progress <= 0.0 then
                self.State    = STATE_AT_HOME
                self.Progress = 0.0
            end
        end
    else
        -- self.State is STATE_AT_HOME or STATE_AT_DEST, so there is
        -- nothing to do but to wait for being activated again.
    end
end
