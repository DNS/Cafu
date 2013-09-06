local Butterfly = ...


function printTab(t)
    for key, value in pairs(t) do
        Console.Print(tostring(key) .. " := " .. tostring(value) .. "\n")
    end
    Console.Print("\n")
end


Butterfly.ArcCenter = { Butterfly:GetEntity():GetTransform():get("Origin") }
Butterfly.ArcRadius = 20.0
Butterfly.ArcPos    =  0.0


function Butterfly:Think(FrameTime)
    -- Console.Print("Butterfly-Thinking...\n")

    local RadPerSecond = math.rad(40.0)   -- The angular speed on the arc is 40 degrees per second.

    self.ArcPos = (self.ArcPos + RadPerSecond * FrameTime) % (math.pi * 2.0)

    self:GetEntity():GetTransform():set("Origin",
        self.ArcCenter[1] + math.sin(self.ArcPos) * self.ArcRadius,
        self.ArcCenter[2] + math.cos(self.ArcPos) * self.ArcRadius,
        self.ArcCenter[3] + math.sin(self.ArcPos * 2.0) * self.ArcRadius * 0.2)

 -- m_Heading = m_ArcPos + math.pi/2.0;
end


Console.Print("####   Hello from Butterfly.lua   ####   :-)\n")
Console.Print("Butterfly tables:\n")

printTab(Butterfly)
printTab(Butterfly.ArcCenter)
