local Beretta        = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = Beretta:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE1         = 0
local ANIM_IDLE2         = 1
local ANIM_IDLE3         = 2
local ANIM_SHOOT         = 3
local ANIM_SHOOT_EMPTY   = 4
local ANIM_RELOAD        = 5
local ANIM_RELOAD_NOSHOT = 6
local ANIM_DRAW          = 7
local ANIM_HOLSTER       = 8


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
        Console.Print("Beretta DRAW sequence wrapped, switching to idle.\n")
        Model:set("Animation", ANIM_IDLE1)
        return
    end

    if SequNr == ANIM_HOLSTER then
        Console.Print("Beretta HOLSTER sequence wrapped, selecting next weapon.\n")
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_IDLE1 then
        Console.Print("Beretta IDLE1 sequence wrapped.\n")
        return
    end
end


function Beretta:IsIdle()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    return Sequ == ANIM_IDLE1 or Sequ == ANIM_IDLE2 or Sequ == ANIM_IDLE3
end


function Beretta:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Beretta:Holster()
    Update1stPersonModel()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function Beretta:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Beretta earlier, and our inventory already has the
        -- maximum number of 9mm bullets, don't pick up anything.
        if Inventory:CheckMax("Bullets9mm") then
            return false
        end

        Inventory:Add("Bullets9mm", 2 * self:get("MaxPrimaryAmmo"))
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        Inventory:Add("Bullets9mm", self:get("MaxPrimaryAmmo"))

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv Bullets9mm:   " .. Inventory:get("Bullets9mm") .. "\n")
    return true
end


function Beretta:GetCrosshairInfo()
    return "Gui/CrossHair1"
end


function Beretta:GetAmmoString()
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("Bullets9mm"))
end
