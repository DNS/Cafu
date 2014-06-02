local Bazooka        = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = Bazooka:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE          = 0
local ANIM_FIDGET        = 1
local ANIM_RELOAD        = 2
local ANIM_FIRE          = 3
local ANIM_HOLSTER       = 4
local ANIM_DRAW          = 5
local ANIM_HOLSTER_EMPTY = 6
local ANIM_DRAW_EMPTY    = 7
local ANIM_IDLE_EMPTY    = 8
local ANIM_FIDGET_EMPTY  = 9


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
        Console.Print("Bazooka DRAW sequence wrapped, switching to idle.\n")
        Model:set("Animation", ANIM_IDLE)
    elseif SequNr == ANIM_IDLE then
        Console.Print("Bazooka IDLE sequence wrapped.\n")
    elseif SequNr == ANIM_HOLSTER then
        Console.Print("Bazooka HOLSTER sequence wrapped, selecting next weapon.\n")
        HumanPlayer:SelectNextWeapon()
    end
end


function Bazooka:IsIdle()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    return Sequ == ANIM_IDLE or Sequ == ANIM_FIDGET or Sequ == ANIM_IDLE_EMPTY or Sequ == ANIM_FIDGET_EMPTY
end


function Bazooka:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Bazooka:Holster()
    Update1stPersonModel()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


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
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        -- Inventory:Add("Rockets", 1)

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv Rockets:   " .. Inventory:get("Rockets") .. "\n")
    return true
end


function Bazooka:GetCrosshairInfo()
    return "Gui/CrossHair2", true
end


function Bazooka:GetAmmoString()
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("Rockets") or 0)
end
