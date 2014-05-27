local AR          = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = AR:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function AR:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Assault Rifle earlier, and our inventory already has the
        -- maximum number of 9mm bullets and AR grenades, don't pick up anything.
        if Inventory:CheckMax("Bullets9mm") and Inventory:CheckMax("ARGrenades") then
            return false
        end

        Inventory:Add("Bullets9mm", 2 * self:get("MaxPrimaryAmmo"))
        Inventory:Add("ARGrenades", 2 * self:get("MaxSecondaryAmmo"))
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo",   self:get("MaxPrimaryAmmo"))
        self:set("SecondaryAmmo", self:get("MaxSecondaryAmmo"))

        Inventory:Add("Bullets9mm", self:get("MaxPrimaryAmmo"))
        Inventory:Add("ARGrenades", self:get("MaxSecondaryAmmo"))
    end

    -- Console.Print("primary   ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("secondary ammo: " .. self:get("SecondaryAmmo") .. "\n")
    -- Console.Print("inv Bullets9mm:   " .. Inventory:get("Bullets9mm") .. "\n")
    -- Console.Print("inv ARGrenades:   " .. Inventory:get("ARGrenades") .. "\n")
    return true
end
