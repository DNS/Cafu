local Shotgun = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.

Console.Print("Shotgun script loaded.\n")


function Shotgun:PickedUp()
    Console.Print("The shotgun was picked up!\n")

--[[
    local InvShells = Inventory:get("Shells")

    if self:get("IsAvail") then
        -- If we already have the shotgun, and our inventory already has the maximum number of shells,
        -- don't pick up anything.
        if InvShells >= Inventory:get("MaxShells") then
            return false
        end

        InvShells = InvShells + 16
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary", 8)

        InvShells = InvShells + 8
    end

    Inventory:set("Shells", math.min(InvShells, Inventory:get("MaxShells")))    --]]
    return true
end
