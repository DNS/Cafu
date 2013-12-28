local FaceHugger = ...  -- Retrieve the ComponentScriptT instance that is responsible for this script.

local Trafo = FaceHugger:GetEntity():GetTransform()
local Model = FaceHugger:GetEntity():GetComponent("Model")
local PaSys = FaceHugger:GetEntity():GetComponent("ParticleSystemOld")


-- There is no need to call InitClientApprox() here, because FaceHuggers have
-- a PlayerPhysics component that sets interpolation for the origin as well.
if false then
    -- TODO: Call InitClientApprox() in some client-init (e.g. OnClientInit()) only?
    Trafo:InitClientApprox("Origin")
end


-- The Model component automatically calls this method whenever the current
-- animation sequence "wraps" (the sequence must be defined as "looping" for
-- this to happen).
function Model:OnSequenceWrap()
    -- Console.Print("Emitting particle...\n")
    PaSys:EmitParticle()
end
