local DesertEagle    = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = DesertEagle:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE1   = 0
local ANIM_FIDGET1 = 1
local ANIM_FIRE    = 2
local ANIM_RELOAD  = 3
local ANIM_HOLSTER = 4
local ANIM_DRAW    = 5
local ANIM_IDLE2   = 6
local ANIM_IDLE3   = 7


local function Update1stPersonModel()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:GetChildren()[2]:GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local self   = DesertEagle
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Model:set("Animation", ANIM_IDLE1)
        return
    end

    if SequNr == ANIM_HOLSTER then
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_RELOAD then
        -- assert(self:CanReload())
        local Amount = math.min(self:get("MaxPrimaryAmmo") - self:get("PrimaryAmmo"), Inventory:get("Bullets357"))

        Inventory:Add("Bullets357", -Amount)
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") + Amount)

        Model:set("Animation", ANIM_IDLE1)
        return
    end

    if SequNr == ANIM_FIRE then
        if self:get("PrimaryAmmo") < 1 and self:CanReload() then
            Model:set("Animation", ANIM_RELOAD)
        else
            Model:set("Animation", ANIM_IDLE1)
        end
        return
    end

    if SequNr == ANIM_IDLE1 or SequNr == ANIM_IDLE2 or SequNr == ANIM_IDLE3 or SequNr == ANIM_FIDGET1 then
        local r = HumanPlayer:GetRandom(4)

        if r == 0 then
            Model:set("Animation", ANIM_IDLE1)
        elseif r == 1 then
            Model:set("Animation", ANIM_IDLE2)
        elseif r == 2 then
            Model:set("Animation", ANIM_IDLE3)
        else
            Model:set("Animation", ANIM_FIDGET1)
        end
        return
    end
end


function DesertEagle:IsIdle()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    return Sequ == ANIM_IDLE1 or Sequ == ANIM_IDLE2 or Sequ == ANIM_IDLE3
end


function DesertEagle:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function DesertEagle:Holster()
    Update1stPersonModel()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function DesertEagle:CanReload()
    return self:get("PrimaryAmmo") < self:get("MaxPrimaryAmmo") and Inventory:get("Bullets357") > 0
end


function DesertEagle:FirePrimary()
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


function DesertEagle:FireSecondary()
    -- No secondary fire for this weapon.
end


function DesertEagle:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the DesertEagle earlier, and our inventory already has the
        -- maximum number of 357 bullets, don't pick up anything.
        if Inventory:CheckMax("Bullets357") then
            return false
        end

        Inventory:Add("Bullets357", 2 * self:get("MaxPrimaryAmmo"))
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        Inventory:Add("Bullets357", self:get("MaxPrimaryAmmo"))

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv Bullets357:   " .. Inventory:get("Bullets357") .. "\n")
    return true
end


function DesertEagle:GetCrosshairInfo()
    return "Gui/CrossHair1"
end


function DesertEagle:GetAmmoString()
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("Bullets357"))
end
