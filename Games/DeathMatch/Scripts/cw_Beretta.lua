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


local function UpdateChildComponents()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:FindByName("FirstPerson"):GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local self   = Beretta
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Model:set("Animation", ANIM_IDLE3)
        return
    end

    if SequNr == ANIM_HOLSTER then
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_RELOAD then
        -- assert(self:CanReload())
        local Amount = math.min(self:get("MaxPrimaryAmmo") - self:get("PrimaryAmmo"), Inventory:get("Bullets9mm"))

        Inventory:Add("Bullets9mm", -Amount)
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") + Amount)

        Model:set("Animation", ANIM_IDLE3)
        return
    end

    if SequNr == ANIM_SHOOT or SequNr == ANIM_SHOOT_EMPTY then
        if self:get("PrimaryAmmo") < 1 and self:CanReload() then
            Model:set("Animation", ANIM_RELOAD)
        else
            Model:set("Animation", ANIM_IDLE3)
        end
        return
    end

    if SequNr == ANIM_IDLE3 then
        local r = HumanPlayer:GetRandom(4)

        if r == 0 then
            Model:set("Animation", ANIM_IDLE1)
        elseif r == 1 then
            Model:set("Animation", ANIM_IDLE2)
        end
        return
    end

    if SequNr == ANIM_IDLE1 or SequNr == ANIM_IDLE2 then
        -- Never play the "nervous" idle animations repeatedly.
        Model:set("Animation", ANIM_IDLE3)
        return
    end
end


function Beretta:PreCache()
    UpdateChildComponents()

    self:Draw()
    Model1stPerson:set("Show", false)
    Model1stPerson:set("Name", "")
end


function Beretta:GetGroup()
    -- The weapon group and keyboard key that this weapon shares with other weapons.
    return 2
end


function Beretta:IsIdle()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_IDLE1 or SequNr == ANIM_IDLE2 or SequNr == ANIM_IDLE3
end


function Beretta:Draw()
    UpdateChildComponents()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", "Games/DeathMatch/Models/Weapons/Beretta/Beretta_v.cmdl")
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Beretta:Holster()
    UpdateChildComponents()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function Beretta:CanReload()
    return self:get("PrimaryAmmo") < self:get("MaxPrimaryAmmo") and Inventory:get("Bullets9mm") > 0
end


function Beretta:FirePrimary(ThinkingOnServerSide)
    if not self:IsIdle() then return end

    if self:get("PrimaryAmmo") < 1 then
        if self:CanReload() then
            Model1stPerson:set("Animation", ANIM_RELOAD)
        else
            Model1stPerson:set("Animation", ANIM_SHOOT_EMPTY)
        end
    else
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") - 1)

     -- WeaponSound:set("Name", "Weapon/DesertEagle_Shot1")
     -- PlayerScript:PostEvent(PlayerScript.EVENT_TYPE_PRIMARY_FIRE)

        Model1stPerson:set("Animation", ANIM_SHOOT)

        if ThinkingOnServerSide then
            HumanPlayer:FireRay(2.0)
        end
    end
end


function Beretta:FireSecondary()
    -- No secondary fire for this weapon.
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
