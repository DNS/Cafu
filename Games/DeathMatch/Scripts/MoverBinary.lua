--[[
The MoverBinary.lua script is intended for use with "binary" movers, that is, with movers that
move between a "home" and "destination" position on a straight line.

A mover can consist of multiple parts, e.g. the wings of a door. Each part must be defined as a
child entity of "this" entity (i.e. the entity that holds the Script and Mover components).
In the Map Editor, all parts are modelled at their initial "home" position. Each part must also
have a child entity of its own (a grandchild of "this" entity). This child entity defines the
"destination" position, i.e. the opposite end of the move at which part will halt after it has
left the "home" position.
--]]

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

-- TODO: Call InitClientApprox() in some client-init (e.g. OnClientInit()) only?
Trafo:InitClientApprox("Origin")
-- Trafo:InitClientApprox("Orientation")


function Script:OnInit()
    self.Parts = self:GetEntity():GetChildren()
    self.HOME_ORIGINS = {}
    self.DEST_ORIGINS = {}

    for i, Part in ipairs(self.Parts) do
        local DestMarker = Part:GetChildren()[1]

        if DestMarker then
            -- The home and dest origins must be determined while Part is still at its home position.
            self.HOME_ORIGINS[i] = Vector3T(Part:GetTransform():GetOriginWS())
            self.DEST_ORIGINS[i] = Vector3T(DestMarker:GetTransform():GetOriginWS())
        end
    end
end


function Script:OnActivate(OtherEnt)
    if self.State ~= STATE_AT_HOME then
        -- If we're moving already, do nothing else until we're done.
        return
    end

    Console.Print("Hello from " .. self:GetEntity():GetBasics():get("Name") .. "'s OnActivate(), called by " .. OtherEnt:GetBasics():get("Name") .. "!\n")

    self.State    = STATE_MOVING_TO_DEST
    self.Progress = 0.0
end


function Script:OnTrigger(OtherEnt)
    if self.State ~= STATE_AT_HOME then
        -- If we're moving already, do nothing else until we're done.
        return
    end

    Console.Print("Hello from " .. self:GetEntity():GetBasics():get("Name") .. "'s OnTrigger(), triggered by " .. OtherEnt:GetBasics():get("Name") .. "!\n")

    self.State    = STATE_MOVING_TO_DEST
    self.Progress = 0.0
end


function Script:GetMove(PartNr, FrameTime)
    if self.State == STATE_AT_HOME or
       self.State == STATE_AT_DEST then
        return
    end

    if not self.HOME_ORIGINS[PartNr] then
        return
    end

    local p

    if self.State == STATE_MOVING_TO_DEST then
        p = math.min(self.Progress + FrameTime / self.Duration, 1.0)
    elseif self.State == STATE_MOVING_BACK then
        p = math.max(self.Progress - FrameTime / self.Duration, 0.0)
    end

    local Part   = self.Parts[PartNr]
    local NewPos = self.HOME_ORIGINS[PartNr] * (1.0 - p) + self.DEST_ORIGINS[PartNr] * p
    local Delta  = NewPos - Vector3T(Part:GetTransform():GetOriginWS())

    return Part:GetID(), Delta[1], Delta[2], Delta[3]
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
