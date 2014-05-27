local DartGun     = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = DartGun:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function DartGun:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the DartGun earlier, and our inventory already has the
        -- maximum number of arrows, don't pick up anything.
        if Inventory:CheckMax("Arrows") then
            return false
        end

        Inventory:Add("Arrows", 2 * self:get("MaxAmmoPrimary"))
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary", self:get("MaxAmmoPrimary"))

        Inventory:Add("Arrows", self:get("MaxAmmoPrimary"))
    end

    -- Console.Print("primary Ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("inv Arrows:   " .. Inventory:get("Arrows") .. "\n")
    return true
end
