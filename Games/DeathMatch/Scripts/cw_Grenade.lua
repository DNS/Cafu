local Grenade        = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = Grenade:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE    = 0
local ANIM_FIDGET  = 1
local ANIM_PINPULL = 2
local ANIM_THROW1  = 3
local ANIM_THROW2  = 4
local ANIM_THROW3  = 5
local ANIM_HOLSTER = 6
local ANIM_DRAW    = 7


local function Update1stPersonModel()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:GetChildren()[2]:GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local self   = Grenade
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Console.Print("Grenade DRAW sequence wrapped, switching to idle.\n")
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_HOLSTER then
        Console.Print("Grenade HOLSTER sequence wrapped, selecting next weapon.\n")
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr == ANIM_PINPULL then
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") - 1)
        Console.Print("Grenade PINPULL sequence wrapped, throwing...\n")
        Model:set("Animation", ANIM_THROW1)
        return
    end

    if SequNr >= ANIM_THROW1 and SequNr <= ANIM_THROW3  then
        if Inventory:get("HandGrenades") > 0 then
            Inventory:Add("HandGrenades", -1)
            self:set("PrimaryAmmo", self:get("PrimaryAmmo") + 1)

            Console.Print("Grenade THROW sequence wrapped, switching to (re-)DRAW.\n")
            Model:set("Animation", ANIM_DRAW)
        else
            -- This is really stupid, because ANIM_IDLE renders a grenade in our hand,
            -- when we in fact have none.
            -- However, we cannot remain in ANIM_THROW* either, because the player can
            -- only switch weapons when in ANIM_IDLE state.
            -- We'd need an ANIM_IDLE_BARE_HANDED in order to overcome this problem, or:
            --   - stay in ANIM_THROW*,
            --   - implement Holster() to return false if in ANIM_THROW*
            --   - adjust IsIdle() ?
            --   - call HumanPlayer:SelectWeapon(-1) here.
            -- (or have something like HumanPlayer:ForceSelectWeapon(-1) ...)
            Model:set("Animation", ANIM_IDLE)
        end
        return
    end

    if SequNr == ANIM_IDLE then
        Console.Print("Grenade IDLE sequence wrapped.\n")
        return
    end
end


function Grenade:IsIdle()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    return Sequ == ANIM_IDLE or Sequ == ANIM_FIDGET
end


function Grenade:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Grenade:Holster()
    Update1stPersonModel()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function Grenade:FirePrimary()
    if not self:IsIdle() then return end
    if self:get("PrimaryAmmo") < 1 then return end

    Model1stPerson:set("Animation", ANIM_PINPULL)

    -- TODO: send primary fire event
    -- TODO: inflict damage
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
