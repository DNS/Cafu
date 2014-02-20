dofile("Games/DeathMatch/Scripts/Vector3.lua")

local Door = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

Door.State  = "IsClosed"
Door.RelPos = 0.0           -- 0.0 is closed, 1.0 is open.

-- Provide defaults in case none are given in the map script.
Door.OpenTime = 1.0
Door.MoveTime = 1.0


function Door:OnInit()
    -- Initialize the parts that the door is made of.
    -- Note that when we get here, all (custom) scripts of all entities have
    -- been loaded and run, so that we can safely access the part's custom values.
    for PartNr, Part in ipairs(self:GetEntity():GetChildren()) do
        -- TODO: Call InitClientApprox() in some client-init (e.g. OnClientInit()) only?
        Part:GetTransform():InitClientApprox("Origin")
     -- Part:GetTransform():InitClientApprox("Orientation")   -- No swing doors yet.

        Part.ClosedPos = Vector3T(Part:GetTransform():get("Origin"))

        if not Part.MoveVec then
            -- Provide a default in case none was given in the map script.
            Part.MoveVec = { 0.0, 64.0, 0.0 }
        end

        -- Default or not, make sure it's a Vector3T.
        Part.MoveVec = Vector3T(Part.MoveVec)
    end
end


function Door:IsClosed(FrameTime)
    -- When the door is closed, there is really nothing to do but wait for
    -- someone to enter the trigger volume and cause a call to OnTrigger().
end


function Door:IsOpening(FrameTime)
    self.RelPos = self.RelPos + FrameTime / self.MoveTime

    if self.RelPos >= 1.0 then
        self.RelPos = 1.0
        self.State = "IsOpen"
        self.OpenTimer = 0.0
    end

    self:UpdatePartPositions()
end


function Door:IsOpen(FrameTime)
    self.OpenTimer = self.OpenTimer + FrameTime

    if self.OpenTimer >= self.OpenTime then
        self.State = "IsClosing"
    end
end


function Door:IsClosing(FrameTime)
    self.RelPos = self.RelPos - FrameTime / self.MoveTime

    if self.RelPos <= 0.0 then
        self.RelPos = 0.0
        self.State = "IsClosed"
    end

    self:UpdatePartPositions()
end


function Door:OnTrigger(Ent)
    self.State = "IsOpening"
end


function Door:Think(FrameTime)
    -- Delegate to the current state handler.
    self[self.State](self, FrameTime)
end


function Door:UpdatePartPositions()
    for PartNr, Part in ipairs(self:GetEntity():GetChildren()) do
     -- local Pos = self.RelPos     -- linear movement
        local Pos = 0.5 + 0.5 * math.sin((self.RelPos - 0.5) * math.pi)

        Part:GetTransform():set("Origin", Part.ClosedPos + Part.MoveVec * Pos)
    end
end
