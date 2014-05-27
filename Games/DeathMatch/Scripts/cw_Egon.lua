local Egon        = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = Egon:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function Egon:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Egon gun earlier, and our inventory already has the
        -- maximum number of cells, don't pick up anything.
        if Inventory:CheckMax("Cells") then
            return false
        end

        Inventory:Add("Cells", 2 * self:get("MaxAmmoPrimary"))
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary", self:get("MaxAmmoPrimary"))

        Inventory:Add("Cells", self:get("MaxAmmoPrimary"))
    end

    -- Console.Print("primary Ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("inv Cells:   " .. Inventory:get("Cells") .. "\n")
    return true
end
