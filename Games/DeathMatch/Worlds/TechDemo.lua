-- It's nicer to call wait(x) than coroutine.yield(x). 
wait=coroutine.yield;

-- This function reloads and runs this very script again.
-- Useful for working with and testing scripts "online",
-- i.e. while the engine is running and without reloading the map!
function reloadscript()
    dofile("Games/DeathMatch/Worlds/TechDemo.lua");
end




-- This function is called by the GUI script with the "CALL RESCUE" button near the level start.
function OnGui_CallRescueButtonPressed()
    wait(3);
    EmergencyExit_Light1:SetColor(0, 196, 0);
end


function DoubleSlideDoor1_Trigger:OnTrigger(Activator)
    self:Deactivate();      -- No more calls to this function please!

    local left =DoubleSlideDoor1_Left;
    local right=DoubleSlideDoor1_Right;

    -- Open the double door.
    local x1, y1, z1= left:GetOrigin(); left :Translate(x1, y1+1000, z1, 1.5);
    local x2, y2, z2=right:GetOrigin(); right:Translate(x2, y2-1000, z2, 1.5);
    -- Console.Print("(" .. x1 .. ", ".. y1 .. ", " .. z1 .. "), (" .. x2 .. ", " .. y2 .. ", " .. z2 .. ")\n");
    wait(1.5);

    -- Wait some time before closing again.
    wait(3);

    -- Close the double door again.
    left :Translate(x1, y1, z1, 1.5);
    right:Translate(x2, y2, z2, 1.5);
    wait(1.5);

    self:Activate();        -- Ok, engine may call this function again.
end


Console.Print("TechDemo.lua script loaded.\n");
