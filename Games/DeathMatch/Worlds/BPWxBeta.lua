-- It's more readable to call wait(x) rather than coroutine.yield(x). 
wait=coroutine.yield;

-- This function reloads and runs this script again.
-- Useful for working with and testing the script "online",
-- i.e. while the engine is running and without reloading the map!
function reloadscript()
    dofile("Games/DeathMatch/Worlds/BPWxBeta.lua");
end



-- This function is called by the teleporter GUI script when the user wants to teleport.
-- origNr and destNr are numbers like 1 or 5 that indicate from and to which node in the
-- teleportation network the teleportation should occur.
--
-- TODO/FIXME: It would be nice if we could learn from an additional parameter which
--   (player) entity was actually operating the teleporter controls (i.e. who pressed
--   the "Go" button on the related GUI).
function teleport(origNr, destNr)
    -- Prevent re-entrancy in the case that this method is called very quickly in succession.
    -- This can happen with the current EntHumanPlayerT code, which is sort of a bug.
    if (isTeleporting) then return true end

    local ox, oy, oz=_G["info_generic_" .. origNr]:GetOrigin();
    local dx, dy, dz=_G["info_generic_" .. destNr]:GetOrigin();

    Console.Print("Teleporting from node " .. origNr .. " to node " .. destNr .. ", ");
    Console.Print("which is at coordinate (" .. dx .. ", " .. dy .. ", " .. dz .. ").\n");

    -- Teleport all entities with name "Player*" to the destination node.
    for PlayerNr=1, 99 do
        local PlayerEnt=_G["Player" .. PlayerNr];

        if not PlayerEnt then break end

        local px, py, pz=PlayerEnt:GetOrigin();

        local ax=px-ox;
        local ay=py-oy;
        local az=pz-oz;

        if (pz>oz and math.sqrt(ax*ax + ay*ay + az*az) < 2000.0) then
            -- VARIANT 1:
            -- When teleporting PlayerEnt to (dx+ax, dy+ay, dz+az), it is safe to teleport
            -- multiple players all at the same time (continuing the loop), as their
            -- relative positioning doesn't change.
            -- However, depending on the map geometry, teleporting to (dx+ax, dy+ay, dz+az)
            -- could teleport the player into solid matter, as it requires that at the dest
            -- there is as much free space (in the same absolute direction) as at the origin.
            -- PlayerEnt:SetOrigin(dx+ax, dy+ay, dz+az);

            -- VARIANT 2:
            -- When teleporting PlayerEnt to (dx, dy, dz+az), the results are much easier
            -- to foresee, but this requires teleporting only one player at a time.
            PlayerEnt:SetOrigin(dx, dy, dz+az);
            break;

            -- TODO/FIXME: We should check that the destination space is actually free
            --   from other players and entities before we relocate PlayerEnt there!
        end
    end

    isTeleporting=true;
    wait(0.5);
    isTeleporting=false;
end
