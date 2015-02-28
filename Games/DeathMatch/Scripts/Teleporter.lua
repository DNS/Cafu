local Script = ...  -- Retrieve the ComponentScriptT instance that is responsible for this script.


-- This method is usually called by the GUI that is attached to Script:GetEntity():GetComponent("Model")
-- when the user presses its related button.
-- TODO/FIXME: It would be nice if we could learn from an additional parameter which (player) entity was
--   actually operating the teleporter controls (i.e. who pressed the "Go" button).
function Script:Teleport(DestEnt)
    -- Prevent re-entrancy in the case that this method is called very quickly in succession.
    -- This can happen with the current EntHumanPlayerT code (checkme: is this still true?).
    if self.isTeleporting then return true end

    Console.Print("teleporting from " .. self:GetEntity():GetBasics():get("Name") ..
                               " to " .. DestEnt:GetBasics():get("Name") .. "\n")

    local ox, oy, oz = self:GetEntity():GetChildren()[1]:GetTransform():GetOriginWS()
    local dx, dy, dz = DestEnt:GetChildren()[1]:GetTransform():GetOriginWS()

    -- Console.Print("Teleporting from node " .. origNr .. " to node " .. destNr .. ", ");
    -- Console.Print("which is at coordinate (" .. dx .. ", " .. dy .. ", " .. dz .. ").\n");

    -- Teleport all entities with name "Player*" to the destination node.
    for PlayerNr = 1, 99 do
        local PlayerEnt = world:GetRootEntity():FindByName("Player_" .. PlayerNr)

        if not PlayerEnt then break end

        local px, py, pz = PlayerEnt:GetTransform():GetOriginWS()

        local ax = px - ox
        local ay = py - oy
        local az = pz - oz

        if (pz > oz) and (math.sqrt(ax*ax + ay*ay + az*az) < 80.0) then
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
            PlayerEnt:GetTransform():SetOriginWS(dx, dy, dz+az)
            break

            -- TODO/FIXME: We should check that the destination space is actually free
            --   from other players and entities before we relocate PlayerEnt there!
        end
    end

    self.isTeleporting = true
    coroutine.yield(0.5)
    self.isTeleporting = false
end
