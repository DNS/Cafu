local BattleScythe = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity       = BattleScythe:GetEntity()
local HumanPlayer  = Entity:GetComponent("HumanPlayer")


function BattleScythe:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the BattleScythe earlier, don't pick up anything.
        return false
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))   -- This is actually 0.
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    return true
end
