local Bazooka     = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = Bazooka:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function Bazooka:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Bazooka earlier, and our inventory already has the
        -- maximum number of rockets, don't pick up anything.
        if Inventory:CheckMax("Rockets") then
            return false
        end

        Inventory:Add("Rockets", 1)
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary", self:get("MaxAmmoPrimary"))

        -- Inventory:Add("Rockets", 1)
    end

    -- Console.Print("primary Ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("inv Rockets:   " .. Inventory:get("Rockets") .. "\n")
    return true
end
