local FaceHugger  = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity      = FaceHugger:GetEntity()
local HumanPlayer = Entity:GetComponent("HumanPlayer")
local Inventory   = Entity:GetComponent("Inventory")


function FaceHugger:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up FaceHuggers earlier, and our inventory already has the
        -- maximum number of FaceHuggers, don't pick up anything.
        if Inventory:CheckMax("FaceHuggers") then
            return false
        end

        Inventory:Add("FaceHuggers", 1)
    else
        self:set("IsAvail", true)
        self:set("AmmoPrimary", self:get("MaxAmmoPrimary"))

        -- Inventory:Add("FaceHuggers", 1)
    end

    -- Console.Print("primary Ammo: " .. self:get("AmmoPrimary") .. "\n")
    -- Console.Print("inv FaceHuggers:   " .. Inventory:get("FaceHuggers") .. "\n")
    return true
end
