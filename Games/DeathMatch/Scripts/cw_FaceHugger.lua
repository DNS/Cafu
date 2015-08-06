local FaceHugger     = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = FaceHugger:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local PlayerScript   = Entity:GetComponent("Script")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil
local WeaponSound    = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE      = 0
local ANIM_FIDGETFIT = 1
local ANIM_FIDGETNIP = 2
local ANIM_HOLSTER   = 3
local ANIM_DRAW      = 4
local ANIM_THROW     = 5


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
    local self   = FaceHugger
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_HOLSTER then
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_THROW then
        if Inventory:get("FaceHuggers") > 0 then
            Inventory:Add("FaceHuggers", -1)
            self:set("PrimaryAmmo", self:get("PrimaryAmmo") + 1)

            Model:set("Animation", ANIM_DRAW)
        else
            -- We have thrown our last face-hugger, but unfortunately, we have no bare-handed animation sequences.
            -- Therefore, we cannot switch to ANIM_IDLE here: It would render a face-hugger in our hand when we
            -- in fact have none.
            -- At the moment, it seems like the only sensible course of action is this:
            HumanPlayer:SelectWeapon(0, true)
            self:set("IsAvail", false)      -- Drawing this weapon "empty" is not possible either.
        end
        return
    end

    if SequNr == ANIM_IDLE then
        local r = HumanPlayer:GetRandom(4)

        if r == 0 then
            Model:set("Animation", ANIM_FIDGETFIT)
        elseif r == 1 then
            Model:set("Animation", ANIM_FIDGETNIP)
        end
        return
    end

    if SequNr == ANIM_FIDGETFIT or SequNr == ANIM_FIDGETNIP then
        -- Never play the "fidget" animations repeatedly.
        Model:set("Animation", ANIM_IDLE)
        return
    end
end


function FaceHugger:PreCache()
    UpdateChildComponents()

    self:Draw()
    Model1stPerson:set("Show", false)
    Model1stPerson:set("Name", "")
end


function FaceHugger:GetGroup()
    -- The weapon group and keyboard key that this weapon shares with other weapons.
    return 5
end


function FaceHugger:IsIdle()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_IDLE or SequNr == ANIM_FIDGETFIT or SequNr == ANIM_FIDGETNIP
end


function FaceHugger:Draw()
    UpdateChildComponents()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", "Games/DeathMatch/Models/Weapons/FaceHugger/FaceHugger_v.cmdl")
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function FaceHugger:Holster()
    UpdateChildComponents()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function FaceHugger:FirePrimary(ThinkingOnServerSide)
    if not self:IsIdle() then return end
    if self:get("PrimaryAmmo") < 1 then return end

    self:set("PrimaryAmmo", self:get("PrimaryAmmo") - 1)

    WeaponSound:set("Name", "Weapon/FaceHugger_Throw")
    PlayerScript:PostEvent(PlayerScript.EVENT_TYPE_PRIMARY_FIRE)

    Model1stPerson:set("Animation", ANIM_THROW)

    if ThinkingOnServerSide then
        -- Spawn a new face-hugger!
        local NewEnt = HumanPlayer:SpawnWeaponChild("FaceHugger")

        local c1 = world:new("ComponentModelT")
        c1:set("Name", "Games/DeathMatch/Models/LifeForms/FaceHugger/FaceHugger.cmdl")

        local c2 = world:new("ComponentParticleSystemOldT")
        c2:set("Type", "FaceHugger")

        local c3 = world:new("ComponentScriptT")    -- Note that any post-load stuff is automatically run by the `CaServerWorldT` implementation.
        c3:set("Name", "Games/DeathMatch/Scripts/FaceHugger.lua")

        NewEnt:AddComponent(c1, c2, c3)
    end
end


function FaceHugger:FireSecondary()
    -- No secondary fire for this weapon.
end


function FaceHugger:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up FaceHuggers earlier, and our inventory already has the
        -- maximum number of FaceHuggers, don't pick up anything.
        if Inventory:CheckMax("FaceHuggers") then
            return false
        end

        Inventory:Add("FaceHuggers", 1)
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        Inventory:Add("FaceHuggers", 1)     -- Make sure that the "FaceHuggers" key is initialized.

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv FaceHuggers:   " .. Inventory:get("FaceHuggers") .. "\n")
    return true
end


function FaceHugger:GetAmmoString()
    return string.format("Ammo %2u (%2u)", self:get("PrimaryAmmo"), Inventory:get("FaceHuggers"))
end


function FaceHugger:ProcessEvent(EventType, NumEvents)
    -- Note that we can *not* have code like
    --     WeaponSound:set("Name", ...)
    -- here, because that would only act on the client-side. The value would be "updated" in
    -- the next client frame with the last value from the server, causing the sound to abort.
    WeaponSound:Play()
end
