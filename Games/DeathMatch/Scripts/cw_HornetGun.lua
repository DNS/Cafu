local HornetGun   = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = HornetGun:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")


function HornetGun:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the HornetGun earlier, don't pick up anything.
        return false
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    return true
end
