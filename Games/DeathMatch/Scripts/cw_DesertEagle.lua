local DesertEagle    = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = DesertEagle:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local PlayerScript   = Entity:GetComponent("Script")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil
local WeaponSound    = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE1   = 0
local ANIM_FIDGET1 = 1
local ANIM_FIRE    = 2
local ANIM_RELOAD  = 3
local ANIM_HOLSTER = 4
local ANIM_DRAW    = 5
local ANIM_IDLE2   = 6
local ANIM_IDLE3   = 7


local function UpdateChildComponents()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:FindByName("FirstPerson"):GetComponent("Model")
    end

    if not WeaponSound then
        WeaponSound = Entity:FindByName("WeaponSound"):GetComponent("Sound")
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


function DesertEagle:PreCache()
    UpdateChildComponents()

    self:Draw()
    Model1stPerson:set("Show", false)
    Model1stPerson:set("Name", "")
end


function DesertEagle:GetGroup()
    -- The weapon group and keyboard key that this weapon shares with other weapons.
    return 2
end


function DesertEagle:IsIdle()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_IDLE1 or SequNr == ANIM_IDLE2 or SequNr == ANIM_IDLE3 or SequNr == ANIM_FIDGET1
end


function DesertEagle:Draw()
    UpdateChildComponents()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", "Games/DeathMatch/Models/Weapons/DesertEagle/DesertEagle_v.cmdl")
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function DesertEagle:Holster()
    UpdateChildComponents()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function DesertEagle:CanReload()
    return self:get("PrimaryAmmo") < self:get("MaxPrimaryAmmo") and Inventory:get("Bullets357") > 0
end


function DesertEagle:FirePrimary(ThinkingOnServerSide)
    if not self:IsIdle() then return end

    if self:get("PrimaryAmmo") < 1 then
        if self:CanReload() then
            Model1stPerson:set("Animation", ANIM_RELOAD)
        end
    else
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") - 1)

        WeaponSound:set("Name", "Weapon/DesertEagle_Shot1")
        PlayerScript:PostEvent(PlayerScript.EVENT_TYPE_PRIMARY_FIRE)

        Model1stPerson:set("Animation", ANIM_FIRE)

        if ThinkingOnServerSide then
            HumanPlayer:FireRay(7.0)
        end
    end
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


function DesertEagle:ProcessEvent(EventType, NumEvents)
    HumanPlayer:RegisterParticle("DesertEagle-ray")

    -- Note that we can *not* have code like
    --     WeaponSound:set("Name", ...)
    -- here, because that would only act on the client-side. The value would be "updated" in
    -- the next client frame with the last value from the server, causing the sound to abort.
    WeaponSound:Play()
end
