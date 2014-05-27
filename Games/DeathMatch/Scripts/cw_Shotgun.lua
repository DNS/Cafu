local Shotgun   = ...   -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity    = Shotgun:GetEntity()
local Inventory = Entity:GetComponent("Inventory")

Console.Print("Shotgun script loaded.\n")


function Shotgun:PickedUp()
    if self:get("IsAvail") then
        -- If we already have the shotgun, and our inventory already has the maximum number of shells,
        -- don't pick up anything.
        if Inventory:CheckMax("Shells") then
            return false
        end

        Inventory:Add("Shells", 16)
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        Inventory:Add("Shells", 8)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv shells:   " .. Inventory:get("Shells") .. "\n")
    return true
end
