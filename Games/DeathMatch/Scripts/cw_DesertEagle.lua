local DesertEagle = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = DesertEagle:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function DesertEagle:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the DesertEagle earlier, and our inventory already has the
        -- maximum number of 357 bullets, don't pick up anything.
        if Inventory:CheckMax("Bullets357") then
            return false
        end

        Inventory:Add("Bullets357", 2 * self:get("MaxAmmoPrimary"))
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary", self:get("MaxAmmoPrimary"))

        Inventory:Add("Bullets357", self:get("MaxAmmoPrimary"))
    end

    -- Console.Print("primary Ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("inv Bullets357:   " .. Inventory:get("Bullets357") .. "\n")
    return true
end
