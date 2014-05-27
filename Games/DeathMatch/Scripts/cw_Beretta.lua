local Beretta     = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = Beretta:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function Beretta:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Beretta earlier, and our inventory already has the
        -- maximum number of 9mm bullets, don't pick up anything.
        if Inventory:CheckMax("Bullets9mm") then
            return false
        end

        Inventory:Add("Bullets9mm", 2 * self:get("MaxAmmoPrimary"))
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary", self:get("MaxAmmoPrimary"))

        Inventory:Add("Bullets9mm", self:get("MaxAmmoPrimary"))
    end

    -- Console.Print("primary Ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("inv Bullets9mm:   " .. Inventory:get("Bullets9mm") .. "\n")
    return true
end
