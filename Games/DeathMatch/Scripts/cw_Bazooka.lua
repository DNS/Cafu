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


local function UpdateChildComponents()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:FindByName("FirstPerson"):GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local self   = Bazooka
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_DRAW_EMPTY then
        Model:set("Animation", ANIM_IDLE_EMPTY)
        return
    end

    if SequNr == ANIM_HOLSTER or SequNr == ANIM_HOLSTER_EMPTY then
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_RELOAD then
        -- assert(self:CanReload())
        Inventory:Add("Rockets", -1)
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") + 1)

        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_FIRE then
        if self:get("PrimaryAmmo") < 1 and self:CanReload() then
            Model:set("Animation", ANIM_RELOAD)
        else
            if self:get("PrimaryAmmo") < 1 then
                Model:set("Animation", ANIM_IDLE_EMPTY)
            else
                Model:set("Animation", ANIM_IDLE)
            end
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

    if SequNr == ANIM_IDLE_EMPTY then
        if HumanPlayer:GetRandom(2) == 0 then
            Model:set("Animation", ANIM_FIDGET_EMPTY)
        end
        return
    end

    if SequNr == ANIM_FIDGET_EMPTY then
        -- Never play the "fidget" animation repeatedly.
        Model:set("Animation", ANIM_IDLE_EMPTY)
        return
    end
end


function Bazooka:PreCache()
    UpdateChildComponents()

    self:Draw()
    Model1stPerson:set("Show", false)
    Model1stPerson:set("Name", "")
end


function Bazooka:GetGroup()
    -- The weapon group and keyboard key that this weapon shares with other weapons.
    return 4
end


function Bazooka:IsIdle()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_IDLE or SequNr == ANIM_FIDGET or SequNr == ANIM_IDLE_EMPTY or SequNr == ANIM_FIDGET_EMPTY
end


function Bazooka:Draw()
    UpdateChildComponents()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", "Games/DeathMatch/Models/Weapons/Bazooka/Bazooka_v.cmdl")

    if self:get("PrimaryAmmo") > 0 then
        Model1stPerson:set("Animation", ANIM_DRAW)
    else
        Model1stPerson:set("Animation", ANIM_DRAW_EMPTY)
    end

    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Bazooka:Holster()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    if SequNr == ANIM_IDLE_EMPTY or SequNr == ANIM_FIDGET_EMPTY then
        Model1stPerson:set("Animation", ANIM_HOLSTER_EMPTY)
    else
        Model1stPerson:set("Animation", ANIM_HOLSTER)
    end

    return true
end


function Bazooka:CanReload()
    return self:get("PrimaryAmmo") < self:get("MaxPrimaryAmmo") and Inventory:get("Rockets") > 0
end


function Bazooka:FirePrimary(ThinkingOnServerSide)
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
            -- Spawn a new rocket (RPG)!
            local NewEnt = HumanPlayer:SpawnWeaponChild("Rocket")

            local c1 = world:new("ComponentModelT")
            c1:set("Name", "Games/DeathMatch/Models/Weapons/Grenade/Grenade_w.cmdl")

            local c2 = world:new("ComponentParticleSystemOldT")
            c2:set("Type", "Rocket_Expl_main")

            local c3 = world:new("ComponentParticleSystemOldT")
            c3:set("Type", "Rocket_Expl_sparkle")

            local c4 = world:new("ComponentPointLightT")
            c4:set("On", true)
            c4:set("Color", 1.0, 0.9, 0.0)
            c4:set("Radius", 500.0)
            -- Shadows are activated only at the time of the explosion (when the model is hidden),
            -- because at this time, our light source origin is at the center of the model, which does not
            -- agree well with shadow casting. Proper solution can be:
            --   - exempt the model from casting shadows,
            --   - offset the light source from the model center, e.g. by `-ViewDir * 16.0`.
            -- The latter is what we did in pre-component-system versions of the code, but now it would
            -- require the employment of a child entity.
            c4:set("ShadowType", 0)     -- No shadows (initially).

            local c5 = world:new("ComponentSoundT")
            c5:set("Name", "Weapon/Shotgun_dBarrel")
            c5:set("AutoPlay", false)

            local c6 = world:new("ComponentScriptT")    -- Note that any post-load stuff is automatically run by the `CaServerWorldT` implementation.
            c6:set("Name", "Games/DeathMatch/Scripts/Rocket.lua")

            NewEnt:AddComponent(c1, c2, c3, c4, c5, c6)
        end
    end
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
