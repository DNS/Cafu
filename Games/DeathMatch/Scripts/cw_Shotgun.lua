local Shotgun   = ...   -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity    = Shotgun:GetEntity()
local Inventory = Entity:GetComponent("Inventory")

Console.Print("Shotgun script loaded.\n")


function Shotgun:PickedUp()
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

    Inventory:set("Shells", math.min(InvShells, Inventory:get("MaxShells")))

    -- Console.Print("primary Ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("inv shells:   " .. Inventory:get("Shells") .. "\n")
    return true
end
