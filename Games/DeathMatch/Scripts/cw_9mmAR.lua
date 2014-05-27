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

        Inventory:Add("Bullets9mm", 2 * self:get("MaxAmmoPrimary"))
        Inventory:Add("ARGrenades", 2 * self:get("MaxAmmoSecondary"))
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary",   self:get("MaxAmmoPrimary"))
        self:set("AmmoSecondary", self:get("MaxAmmoSecondary"))

        Inventory:Add("Bullets9mm", self:get("MaxAmmoPrimary"))
        Inventory:Add("ARGrenades", self:get("MaxAmmoSecondary"))
    end

    -- Console.Print("primary   ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("secondary ammo: " .. self:get("AmmoSecondary") .. "\n")
    -- Console.Print("inv Bullets9mm:   " .. Inventory:get("Bullets9mm") .. "\n")
    -- Console.Print("inv ARGrenades:   " .. Inventory:get("ARGrenades") .. "\n")
    return true
end
