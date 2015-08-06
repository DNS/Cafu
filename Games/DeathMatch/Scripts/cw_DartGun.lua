local DartGun        = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = DartGun:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE1   = 0
local ANIM_IDLE2   = 1
local ANIM_IDLE3   = 2
local ANIM_FIRE    = 3
local ANIM_RELOAD  = 4
local ANIM_DRAW    = 5
local ANIM_HOLSTER = 6


local function UpdateChildComponents()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:FindByName("FirstPerson"):GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local self   = DartGun
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
        -- From the reload sequence, it seems as if the DartGun can only ever hold a single arrow at a time...?
        local Amount = math.min(self:get("MaxPrimaryAmmo") - self:get("PrimaryAmmo"), Inventory:get("Arrows"))

        Inventory:Add("Arrows", -Amount)
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

    if SequNr == ANIM_IDLE1 or SequNr == ANIM_IDLE2 or SequNr == ANIM_IDLE3 then
        local r = HumanPlayer:GetRandom(3)

        if r == 0 then
            Model:set("Animation", ANIM_IDLE1)
        elseif r == 1 then
            Model:set("Animation", ANIM_IDLE2)
        else
            Model:set("Animation", ANIM_IDLE3)
        end
        return
    end
end


function DartGun:PreCache()
    UpdateChildComponents()

    self:Draw()
    Model1stPerson:set("Show", false)
    Model1stPerson:set("Name", "")
end


function DartGun:GetGroup()
    -- The weapon group and keyboard key that this weapon shares with other weapons.
    return 3
end


function DartGun:IsIdle()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_IDLE1 or SequNr == ANIM_IDLE2 or SequNr == ANIM_IDLE3
end


function DartGun:Draw()
    UpdateChildComponents()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", "Games/DeathMatch/Models/Weapons/DartGun/DartGun_v.cmdl")
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function DartGun:Holster()
    UpdateChildComponents()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function DartGun:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the DartGun earlier, and our inventory already has the
        -- maximum number of arrows, don't pick up anything.
        if Inventory:CheckMax("Arrows") then
            return false
        end

        Inventory:Add("Arrows", 2 * self:get("MaxPrimaryAmmo"))
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        Inventory:Add("Arrows", self:get("MaxPrimaryAmmo"))

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv Arrows:   " .. Inventory:get("Arrows") .. "\n")
    return true
end


function DartGun:CanReload()
    return self:get("PrimaryAmmo") < self:get("MaxPrimaryAmmo") and Inventory:get("Arrows") > 0
end


function DartGun:FirePrimary(ThinkingOnServerSide)
    if not self:IsIdle() then return end

    if self:get("PrimaryAmmo") < 1 then
        if self:CanReload() then
            Model1stPerson:set("Animation", ANIM_RELOAD)
        end
    else
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") - 1)

        -- WeaponSound:set("Name", "Weapon/DesertEagle_Shot1")
        -- PlayerScript:PostEvent(PlayerScript.EVENT_TYPE_PRIMARY_FIRE)

        Model1stPerson:set("Animation", ANIM_FIRE)

        if ThinkingOnServerSide then
            HumanPlayer:FireRay(20.0)
        end
    end
end


function DartGun:FireSecondary()
    -- No secondary fire for this weapon.
end


function DartGun:GetCrosshairInfo()
    return "Gui/CrossHair1"
end


function DartGun:GetAmmoString()
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("Arrows"))
end
