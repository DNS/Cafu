local Tripmine    = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = Tripmine:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function Tripmine:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up tripmines earlier, and our inventory already has the
        -- maximum number of tripmines, don't pick up anything.
        if Inventory:CheckMax("Tripmines") then
            return false
        end

        Inventory:Add("Tripmines", 1)
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary", self:get("MaxAmmoPrimary"))

        -- Inventory:Add("Tripmines", 1)
    end

    -- Console.Print("primary Ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("inv Tripmines:   " .. Inventory:get("Tripmines") .. "\n")
    return true
end
