local Grenade        = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = Grenade:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local PlayerScript   = Entity:GetComponent("Script")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil
local WeaponSound    = nil

Grenade.ThinkingOnServerSide = false


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE    = 0
local ANIM_FIDGET  = 1
local ANIM_PINPULL = 2
local ANIM_THROW1  = 3
local ANIM_THROW2  = 4
local ANIM_THROW3  = 5
local ANIM_HOLSTER = 6
local ANIM_DRAW    = 7


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
    local self   = Grenade
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_HOLSTER then
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_PINPULL then
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") - 1)

        -- See the comment in Grenade:ProcessEvent() for more details about the next two lines.
        WeaponSound:set("Name", "Weapon/FaceHugger_Throw")
        PlayerScript:PostEvent(PlayerScript.EVENT_TYPE_PRIMARY_FIRE)

        Model:set("Animation", ANIM_THROW1 + HumanPlayer:GetRandom(3))

        if self.ThinkingOnServerSide then
            -- Spawn a new hand grenade!
            local NewEnt = HumanPlayer:SpawnWeaponChild("Grenade")

            local c1 = world:new("ComponentModelT")
            c1:set("Name", "Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl")

            local c2 = world:new("ComponentParticleSystemOldT")
            c2:set("Type", "HandGrenade_Expl_main")

            local c3 = world:new("ComponentParticleSystemOldT")
            c3:set("Type", "HandGrenade_Expl_sparkle")

            local c4 = world:new("ComponentPointLightT")
            c4:set("On", false)
            c4:set("Color", 1.0, 1.0, 1.0)
            c4:set("Radius", 400.0)
            c4:set("ShadowType", 1)     -- STENCIL shadows

            local c5 = world:new("ComponentSoundT")
            c5:set("Name", "Weapon/Shotgun_dBarrel")
            c5:set("AutoPlay", false)

            local c6 = world:new("ComponentScriptT")    -- Note that any post-load stuff is automatically run by the `CaServerWorldT` implementation.
            c6:set("Name", "Games/DeathMatch/Scripts/Grenade.lua")
            c6:set("ScriptCode", "local Grenade = ...\nGrenade.LightDuration = 0.5\n")

            NewEnt:AddComponent(c1, c2, c3, c4, c5, c6)
        end
        return
    end

    if SequNr >= ANIM_THROW1 and SequNr <= ANIM_THROW3  then
        if Inventory:get("HandGrenades") > 0 then
            Inventory:Add("HandGrenades", -1)
            self:set("PrimaryAmmo", self:get("PrimaryAmmo") + 1)

            Model:set("Animation", ANIM_DRAW)
        else
            -- We have thrown our last grenade, but unfortunately, we have no bare-handed animation sequences.
            -- Therefore, we cannot switch to ANIM_IDLE here: It would render a grenade in our hand when we
            -- in fact have none.
            -- At the moment, it seems like the only sensible course of action is this:
            HumanPlayer:SelectWeapon(0, true)
            self:set("IsAvail", false)      -- Drawing this weapon "empty" is not possible either.
        end
        return
    end

    if SequNr == ANIM_IDLE then
        if HumanPlayer:GetRandom(2) == 0 then
            Model:set("Animation", ANIM_FIDGET)
        end
        return
    end

    if SequNr == ANIM_FIDGET then
        -- Never play the "fidget" animation repeatedly.
        Model:set("Animation", ANIM_IDLE)
        return
    end
end


function Grenade:PreCache()
    UpdateChildComponents()

    self:Draw()
    Model1stPerson:set("Show", false)
    Model1stPerson:set("Name", "")
end


function Grenade:GetGroup()
    -- The weapon group and keyboard key that this weapon shares with other weapons.
    return 5
end


function Grenade:IsIdle()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_IDLE or SequNr == ANIM_FIDGET
end


function Grenade:Draw()
    UpdateChildComponents()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", "Games/DeathMatch/Models/Weapons/Grenade/Grenade_v.cmdl")
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Grenade:Holster()
    UpdateChildComponents()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function Grenade:FirePrimary(ThinkingOnServerSide)
    if not self:IsIdle() then return end
    if self:get("PrimaryAmmo") < 1 then return end

    Model1stPerson:set("Animation", ANIM_PINPULL)

    -- The grenade is only thrown when the ANIM_PINPULL animation is complete,
    -- so we have to save the ThinkingOnServerSide variable until then.
    self.ThinkingOnServerSide = ThinkingOnServerSide
end


function Grenade:FireSecondary()
    -- No secondary fire for this weapon.
end


function Grenade:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up hand grenades earlier, and our inventory already has the
        -- maximum number of hand grenades, don't pick up anything.
        if Inventory:CheckMax("HandGrenades") then
            return false
        end

        Inventory:Add("HandGrenades", 1)
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        Inventory:Add("HandGrenades", 0)    -- Make sure that the "HandGrenades" key is initialized.

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv HandGrenades:   " .. Inventory:get("HandGrenades") .. "\n")
    return true
end


function Grenade:GetAmmoString()
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("HandGrenades"))
end


function Grenade:ProcessEvent(EventType, NumEvents)
    -- If this is called for the first time, the weapon sound is empty (""), even though it seems like
    -- everything has been properly setup in OnSequenceWrap_Sv() above (case "ANIM_PINPULL").
    -- With later calls, everything works as expected.
    -- Similarly to the matter explained in UpdateChildComponents() above, I suspect that this is
    -- a result of the Script component / the event already being processed immediately after it has
    -- been updated from the incoming network message, while the sound name, kept in the Sound component
    -- that in turn is in a *child* entity, has not yet been updated!
    -- Paradoxically, the same seems not to happen to the FaceHugger (or any other weapon), which all seem
    -- to be in a comparable setup.
    Console.Print("Grenade:ProcessEvent(), weapon sound is: \"" .. WeaponSound:get("Name") .. "\"\n")

    -- Note that we can *not* have code like
    --     WeaponSound:set("Name", ...)
    -- here, because that would only act on the client-side. The value would be "updated" in
    -- the next client frame with the last value from the server, causing the sound to abort.
    WeaponSound:Play()
end
