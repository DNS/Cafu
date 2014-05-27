local Grenade     = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = Grenade:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function Grenade:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up hand grenades earlier, and our inventory already has the
        -- maximum number of hand grenades, don't pick up anything.
        if Inventory:CheckMax("HandGrenades") then
            return false
        end

        Inventory:Add("HandGrenades", 1)
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        -- Inventory:Add("HandGrenades", 1)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv HandGrenades:   " .. Inventory:get("HandGrenades") .. "\n")
    return true
end
