dofile("Games/DeathMatch/Scripts/Print.lua")
dofile("Games/DeathMatch/Scripts/Vector3.lua")
-- dofile("Games/DeathMatch/Scripts/Strict.lua")

local Eagle = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

Eagle.Trafo          = Eagle:GetEntity():GetTransform()
Eagle.FlightState    = "CruiseFlight"
Eagle.OldOrigin      = Vector3T()
Eagle.LoopCenter     = Vector3T()
Eagle.FigureDistance = 0.0
Eagle.FigureLeft     = 0.0

Eagle.Trafo:InitClientApprox("Origin")
Eagle.Trafo:InitClientApprox("Orientation")   -- This is well visible (at the wingtips).


function Eagle:GetFlightDir()
    local FlightDir = Vector3T(self.Trafo:GetAxisX())
    FlightDir[3] = 0.0

    return FlightDir
end


function Eagle:CruiseFlight(FlightLeft)
    local Origin    = Vector3T(self.Trafo:get("Origin"))
    local FlightDir = self:GetFlightDir()
    local Result    = world:TraceRay(Origin, FlightDir * 1000.0)

    local TerrainDistance  = 1000.0 * Result.Fraction
    local ManeuverDistance =  280.0     -- The minimum space that we would like to use for the turn maneuver.

    if TerrainDistance > ManeuverDistance + FlightLeft then
        -- Continue cruise flight.
        self.Trafo:set("Origin", Origin + FlightDir * FlightLeft)

        return 0.0
    end

    -- Terrain is ahead, so initiate the turn maneuver.
    -- Use 80% of the remaining space for the entire maneuver, and 1/3 of this for the sections of the figures.
    self.FigureDistance = TerrainDistance * 0.8 / 3.0
    self.OldOrigin      = Origin
    self.OldFlightDir   = FlightDir
    self.OldHeading     = self.Trafo:GetAngles()
    self.LoopCenter     = Origin + 2.0 * self.FigureDistance * FlightDir - Vector3T(0.0, 0.0, self.FigureDistance)

    -- Change state to ControlledCruise.
    self.FlightState = "ControlledCruise"
    self.FigureLeft  = 2.0 * self.FigureDistance

    return FlightLeft
end


function Eagle:ControlledCruise(FlightLeft)
    if FlightLeft > self.FigureLeft then
        FlightLeft = FlightLeft - self.FigureLeft

        -- Change state to HalfLoopAndRoll.
        self.FlightState = "HalfLoopAndRoll"
        self.FigureLeft  = math.pi * self.FigureDistance

        return FlightLeft
    end

    self.FigureLeft = self.FigureLeft - FlightLeft
    self.Trafo:set("Origin", Vector3T(self.Trafo:get("Origin")) + self.OldFlightDir * FlightLeft)

    return 0.0
end


function Eagle:HalfLoopAndRoll(FlightLeft)
    if FlightLeft > self.FigureLeft then
        FlightLeft = FlightLeft - self.FigureLeft

        -- Change state to ClimbBackToCruiseAlt, where a "cosine-upswing" is implemented
        -- back to the original cruise altitude. The shortest path (a straight line) would
        -- be a diagonal of length 2.0 * 1.414213562373 * self.FigureDistance. The cosine
        -- arc is obviously slightly longer, estimate its length at 2.0 * 1.5 * self.FigureDistance
        self.Trafo:SetAngles(self.OldHeading + 180.0, 0.0, 0.0)

        self.FlightState = "ClimbBackToCruiseAlt"
        self.FigureLeft  = 3.0 * self.FigureDistance

        return FlightLeft
    end

    self.FigureLeft = self.FigureLeft - FlightLeft

    local DegreesLeft = self.FigureLeft / self.FigureDistance
    local HorLoopPos  = math.sin(DegreesLeft) * self.FigureDistance
    local VerLoopPos  = math.cos(DegreesLeft) * self.FigureDistance

    self.Trafo:set("Origin", self.LoopCenter + self.OldFlightDir * HorLoopPos - Vector3T(0.0, 0.0, VerLoopPos))
    self.Trafo:SetAngles(self.OldHeading, 180.0 - math.deg(DegreesLeft), 180.0 - math.deg(DegreesLeft))

    return 0.0
end


function Eagle:ClimbBackToCruiseAlt(FlightLeft)
    if FlightLeft > self.FigureLeft then
        FlightLeft = FlightLeft - self.FigureLeft

        -- Change state to CruiseFlight.
        self.Trafo:set("Origin", self.OldOrigin)

        self.FlightState = "CruiseFlight"
        self.FigureLeft  = 0.0   -- Not relevant in CruiseFlight.

        return FlightLeft
    end

    self.FigureLeft = self.FigureLeft - FlightLeft

    local TrackLeft = self.FigureLeft / 1.5                             -- Distance left on the ground track.
    local RadLeft   = TrackLeft / self.FigureDistance * math.pi / 2.0   -- Corresponding to this many radians.

    self.Trafo:set("Origin", self.OldOrigin + self.OldFlightDir * TrackLeft + Vector3T(0.0, 0.0, self.FigureDistance * (math.cos(RadLeft) - 1.0)))

    return 0.0
end


function Eagle:Think(FrameTime)
    local FlightLeft = 160.0 * FrameTime    -- How far we would like to fly.

    -- print("\n-- Think(", FrameTime, ")  -- FlightLeft =", FlightLeft)
    while FlightLeft > 0.0 do
        -- print(self.FlightState, "(", FlightLeft, ")\nOrigin =", self.Trafo:get("Origin"))
        FlightLeft = self[self.FlightState](self, FlightLeft)
    end
end
