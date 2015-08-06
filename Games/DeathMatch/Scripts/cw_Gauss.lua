local Gauss          = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = Gauss:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE    = 0
local ANIM_IDLE2   = 1
local ANIM_FIDGET  = 2
local ANIM_SPINUP  = 3
local ANIM_SPIN    = 4
local ANIM_FIRE    = 5
local ANIM_FIRE2   = 6
local ANIM_HOLSTER = 7
local ANIM_DRAW    = 8


local function UpdateChildComponents()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:FindByName("FirstPerson"):GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_HOLSTER then
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_IDLE or SequNr == ANIM_IDLE2 or SequNr == ANIM_FIDGET then
        local r = HumanPlayer:GetRandom(4)

        if r == 0 then
            Model:set("Animation", ANIM_IDLE2)
        elseif r == 1 then
            Model:set("Animation", ANIM_FIDGET)
        else
            Model:set("Animation", ANIM_IDLE)
        end
        return
    end
end


function Gauss:PreCache()
    UpdateChildComponents()

    self:Draw()
    Model1stPerson:set("Show", false)
    Model1stPerson:set("Name", "")
end


function Gauss:GetGroup()
    -- The weapon group and keyboard key that this weapon shares with other weapons.
    return 4
end


function Gauss:IsIdle()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_IDLE or SequNr == ANIM_IDLE2 or SequNr == ANIM_FIDGET
end


function Gauss:Draw()
    UpdateChildComponents()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", "Games/DeathMatch/Models/Weapons/Gauss/Gauss_v.cmdl")
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Gauss:Holster()
    UpdateChildComponents()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function Gauss:FirePrimary()
    -- No primary fire for this weapon (at this time -- TODO).
end


function Gauss:FireSecondary()
    -- No secondary fire for this weapon.
end


function Gauss:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Gauss gun earlier, and our inventory already has the
        -- maximum number of cells, don't pick up anything.
        if Inventory:CheckMax("Cells") then
            return false
        end

        Inventory:Add("Cells", 2 * self:get("MaxPrimaryAmmo"))
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        Inventory:Add("Cells", self:get("MaxPrimaryAmmo"))

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv Cells:   " .. Inventory:get("Cells") .. "\n")
    return true
end


function Gauss:GetCrosshairInfo()
    return "Gui/CrossHair2", true
end


function Gauss:GetAmmoString()
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("Cells"))
end
