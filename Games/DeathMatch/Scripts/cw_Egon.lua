local Egon           = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = Egon:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE         =  0
local ANIM_FIDGET       =  1
local ANIM_ALTFIREON    =  2
local ANIM_ALTFIRECYCLE =  3
local ANIM_ALTFIREOFF   =  4
local ANIM_FIRE1        =  5
local ANIM_FIRE2        =  6
local ANIM_FIRE3        =  7
local ANIM_FIRE4        =  8
local ANIM_DRAW         =  9
local ANIM_HOLSTER      = 10


local function Update1stPersonModel()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:GetChildren()[2]:GetComponent("Model")
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

    if SequNr == ANIM_IDLE then
        return
    end
end


function Egon:IsIdle()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    return Sequ == ANIM_IDLE or Sequ == ANIM_FIDGET
end


function Egon:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Egon:Holster()
    Update1stPersonModel()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function Egon:FirePrimary()
    -- No primary fire for this weapon (at this time -- TODO).
end


function Egon:FireSecondary()
    -- No secondary fire for this weapon.
end


function Egon:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Egon gun earlier, and our inventory already has the
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


function Egon:GetCrosshairInfo()
    return "Gui/CrossHair2", true
end


function Egon:GetAmmoString()
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("Cells"))
end
