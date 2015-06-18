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
Script.TimeLeft = -1.0              -- This is properly inited whenever STATE_AT_DEST is entered.


function Script:OnInit()
    self.Parts = self:GetEntity():GetChildren()
    self.HOME_ORIGINS = {}
    self.DEST_ORIGINS = {}

    for i, Part in ipairs(self.Parts) do
        Part:GetTransform():InitClientApprox("Origin")
     -- Part:GetTransform():InitClientApprox("Orientation")   -- No swing doors yet.

        local DestMarker = Part:GetChildren()[1]

        if DestMarker then
            -- The home and dest origins must be determined while Part is still at its home position.
            self.HOME_ORIGINS[i] = Vector3T(Part:GetTransform():GetOriginWS())
            self.DEST_ORIGINS[i] = Vector3T(DestMarker:GetTransform():GetOriginWS())
        end
    end
end


function Script:OnActivate(OtherEnt)
    if self.State == STATE_AT_HOME then
        Console.Print(self:GetEntity():GetBasics():get("Name") .. "'s OnActivate() has been called by " .. OtherEnt:GetBasics():get("Name") .. "!\n")

        self.State    = STATE_MOVING_TO_DEST
        self.Progress = 0.0
    elseif self.State == STATE_AT_DEST then
        -- 0 means "move back to home position"
        -- 1 means "reset timeout"
        -- 2 means "ignore being activated, do nothing"
        if Mover:get("destActivated") == 0 then
            Console.Print(self:GetEntity():GetBasics():get("Name") .. "'s OnActivate() has been called by " .. OtherEnt:GetBasics():get("Name") .. "!\n")

            self.State    = STATE_MOVING_BACK
            self.Progress = 1.0
        elseif Mover:get("destActivated") == 1 then
            self.TimeLeft = Mover:get("destTimeout")
        end
    end
end


function Script:OnTrigger(OtherEnt)
    self:OnActivate(OtherEnt)
end


function Script:GetMove(PartNr, FrameTime)
    if self.State == STATE_AT_HOME or
       self.State == STATE_AT_DEST then
        return
    end

    if not self.HOME_ORIGINS[PartNr] then
        return
    end

    --[[
    The key idea is to advance `self.Progress` linearly with the `FrameTime`.
    Its linear progress from 0.0 to 1.0 is then "modulated" by some s-shaped curve
    in order to simulate acceleration and deceleration effects near the endpoints.

    The "s-shaped curves" map 0.0 to 0.0, 1.0 to 1.0, and for symmetry, 0.5 to 0.5.
    Symmetry is not a requirement, but a natural choice. It also makes the actual
    computation of the s-shaped curves much easier, because we only need to compute
    a segment of a curve for the "first half", which is then appropriately mirrored
    and thus re-used for the "second half".
    --]]
    local dp = FrameTime / (Mover:get("moveDuration") or 1.0)
    local p

    if self.State == STATE_MOVING_TO_DEST then
        p = math.min(self.Progress + dp, 1.0)
    elseif self.State == STATE_MOVING_BACK then
        p = math.max(self.Progress - dp, 0.0)
    end

    local exp = Mover:get("trajExp") or 1.0
    local res

    if Mover:get("trajFunc") == 0 then
        -- Use a LINEAR base curve.
        if p < 0.5 then
            res = math.pow(p * 2.0, exp) / 2.0
        else
            res = 1.0 - math.pow((1.0 - p) * 2.0, exp) / 2.0
        end
    elseif Mover:get("trajFunc") == 1 then
        -- Use a SINE base curve.
        if p < 0.5 then
            res = math.pow(1.0 - math.cos(p * math.pi), exp) / 2.0
        else
            res = 1.0 - math.pow(1.0 - math.cos((1.0 - p) * math.pi), exp) / 2.0
        end
    else
        -- Use a linear base curve with exp == 1.0.
        res = p
    end

    local NewPos = self.HOME_ORIGINS[PartNr] * (1.0 - res) + self.DEST_ORIGINS[PartNr] * res
    local Part   = self.Parts[PartNr]
    local Delta  = NewPos - Vector3T(Part:GetTransform():GetOriginWS())

    return Part:GetID(), Delta[1], Delta[2], Delta[3]
end


function Script:Think(FrameTime)
    local dp = FrameTime / (Mover:get("moveDuration") or 1.0)

    if self.State == STATE_MOVING_TO_DEST then
        if Mover:HandleMove(FrameTime) then
            self.Progress = self.Progress + dp

            if self.Progress >= 1.0 then
                self.State    = STATE_AT_DEST
                self.Progress = 1.0
                self.TimeLeft = Mover:get("destTimeout")
            end
        end
    elseif self.State == STATE_MOVING_BACK then
        if Mover:HandleMove(FrameTime) then
            self.Progress = self.Progress - dp

            if self.Progress <= 0.0 then
                self.State    = STATE_AT_HOME
                self.Progress = 0.0
             -- self.TimeLeft = Mover:get("destTimeout")
            end
        end
    elseif self.State == STATE_AT_DEST then
        -- If a `destTimeout` has been specified, run the timer.
        -- Note that this is entirely independent of the `destActivated` setting.
        if Mover:get("destTimeout") >= 0.0 then
            self.TimeLeft = self.TimeLeft - FrameTime

            if self.TimeLeft <= 0.0 then
                self.State    = STATE_MOVING_BACK
                self.Progress = 1.0
            end
        end
    else
        -- self.State is STATE_AT_HOME, so there is
        -- nothing to do but to wait for being activated again.
    end
end
