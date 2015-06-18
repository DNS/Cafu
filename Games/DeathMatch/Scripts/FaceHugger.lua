local FaceHugger = ...  -- Retrieve the ComponentScriptT instance that is responsible for this script.

local Trafo = FaceHugger:GetEntity():GetTransform()
local Model = FaceHugger:GetEntity():GetComponent("Model")
local PaSys = FaceHugger:GetEntity():GetComponent("ParticleSystemOld")
local PlPhy = FaceHugger:GetEntity():GetComponent("PlayerPhysics")

Trafo:InitClientApprox("Origin")


function FaceHugger:Think(FrameTime)
    PlPhy:MoveHuman(FrameTime, 0, 0, 0, 0, 0, 0, false)
end


-- The Model component automatically calls this method whenever the current
-- animation sequence "wraps" (the sequence must be defined as "looping" for
-- this to happen).
function Model:OnSequenceWrap()
    -- Console.Print("Emitting particle...\n")
    PaSys:EmitParticle()
end
