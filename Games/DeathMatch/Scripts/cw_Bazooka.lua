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
    local self   = Bazooka
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Console.Print("Bazooka DRAW sequence wrapped, switching to idle.\n")
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_DRAW_EMPTY then
        Console.Print("Bazooka DRAW_EMPTY sequence wrapped, switching to idle_empty.\n")
        Model:set("Animation", ANIM_IDLE_EMPTY)
        return
    end

    if SequNr == ANIM_HOLSTER or SequNr == ANIM_HOLSTER_EMPTY then
        Console.Print("Bazooka HOLSTER or HOLSTER_EMPTY sequence wrapped, selecting next weapon.\n")
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_RELOAD then
        -- assert(self:CanReload())
        Inventory:Add("Rockets", -1)
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") + 1)

        Console.Print("Bazooka RELOAD sequence wrapped, switching to idle.\n")
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_FIRE then
        if self:get("PrimaryAmmo") < 1 and self:CanReload() then
            Model:set("Animation", ANIM_RELOAD)
        else
            if self:get("PrimaryAmmo") < 1 then
                Console.Print("Bazooka FIRE sequence wrapped, switching to idle_empty.\n")
                Model:set("Animation", ANIM_IDLE_EMPTY)
            else
                Console.Print("Bazooka FIRE sequence wrapped, switching to idle.\n")
                Model:set("Animation", ANIM_IDLE)
            end
        end
        return
    end

    if SequNr == ANIM_IDLE or SequNr == ANIM_IDLE_EMPTY then
        Console.Print("Bazooka IDLE or IDLE_EMPTY sequence wrapped.\n")
        return
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

    if self:get("PrimaryAmmo") > 0 then
        Model1stPerson:set("Animation", ANIM_DRAW)
    else
        Model1stPerson:set("Animation", ANIM_DRAW_EMPTY)
    end

    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Bazooka:Holster()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    if Sequ == ANIM_IDLE_EMPTY or Sequ == ANIM_FIDGET_EMPTY then
        Model1stPerson:set("Animation", ANIM_HOLSTER_EMPTY)
    else
        Model1stPerson:set("Animation", ANIM_HOLSTER)
    end

    return true
end


function Bazooka:CanReload()
    return self:get("PrimaryAmmo") < self:get("MaxPrimaryAmmo") and Inventory:get("Rockets") > 0
end


function Bazooka:FirePrimary()
    if not self:IsIdle() then return end

    if self:get("PrimaryAmmo") < 1 then
        if self:CanReload() then
            Model1stPerson:set("Animation", ANIM_RELOAD)
        end
    else
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") - 1)

        Model1stPerson:set("Animation", ANIM_FIRE)
    end

    -- TODO: send primary fire event
    -- TODO: inflict damage
end


function Bazooka:FireSecondary()
    -- No secondary fire for this weapon.
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

        Inventory:Add("Rockets", 0)     -- Make sure that the "Rockets" key is initialized.

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
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("Rockets"))
end
