local Butterfly = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.

Butterfly.Trafo     = Butterfly:GetEntity():GetTransform()
Butterfly.ArcCenter = { Butterfly.Trafo:get("Origin") }
Butterfly.ArcRadius = 20.0
Butterfly.ArcPos    =  0.0

Butterfly.Trafo:InitClientApprox("Origin")
Butterfly.Trafo:InitClientApprox("Orientation")   -- A bit overkill for tiny butterflies... ;-)


-- If still at default (-1), pick a custom model skin.
local Model = Butterfly:GetEntity():GetComponent("Model")

if Model:get("Skin") == -1 and Model:GetNumSkins() > 0 then
    Model:set("Skin",
        Butterfly:GetEntity():GetID() % Model:GetNumSkins())
end


function Butterfly:Think(FrameTime)
    -- Console.Print("Butterfly-Thinking...\n")

    local RadPerSecond = math.rad(40.0)   -- The angular speed on the arc is 40 degrees per second.

    self.ArcPos = (self.ArcPos + RadPerSecond * FrameTime) % (math.pi * 2.0)

    self.Trafo:set("Origin",
        self.ArcCenter[1] + math.sin(self.ArcPos) * self.ArcRadius,
        self.ArcCenter[2] + math.cos(self.ArcPos) * self.ArcRadius,
        self.ArcCenter[3] + math.sin(self.ArcPos * 2.0) * self.ArcRadius * 0.2)

    self.Trafo:SetAngles(-math.deg(self.ArcPos))
end
