local HumanPlayer    = ...  -- Retrieve the ComponentScriptT instance that is responsible for this script.
local Entity         = HumanPlayer:GetEntity()
local Trafo          = Entity:GetTransform()
local Model3rdPerson = Entity:GetComponent("Model")


function Model3rdPerson:OnAnimationChange(AnimNr)
    Console.Print("OnAnimationChange " .. AnimNr .. "\n")

    if (AnimNr < 18) or (AnimNr > 24) then
        -- The player is "alive", so blend animation sequences in 0.3 seconds,
        -- and force their playback in a loop.
        return 0.3, true
    end

    -- TODO: It would be nice if we also had the number of frame and FPS of the animation sequence,
    -- and could then use   math.min(0.2, (NumFrames - 1) * FPS * 0.5)   as blend time.
    return 0.2, false
end
