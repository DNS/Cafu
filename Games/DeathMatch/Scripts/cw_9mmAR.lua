local AR             = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = AR:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_LONGIDLE = 0
local ANIM_IDLE1    = 1
local ANIM_GRENADE  = 2
local ANIM_RELOAD   = 3
local ANIM_DRAW     = 4
local ANIM_SHOOT1   = 5
local ANIM_SHOOT2   = 6
local ANIM_SHOOT3   = 7


local function UpdateChildComponents()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:FindByName("FirstPersonEnt"):GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local self   = AR
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Model:set("Animation", ANIM_LONGIDLE)
        return
    end

 -- if SequNr == ANIM_HOLSTER then
 --     HumanPlayer:SelectNextWeapon()
 --     return
 -- end

    if SequNr == ANIM_RELOAD then
        -- assert(self:CanReload())
        local Amount1 = math.min(self:get("MaxPrimaryAmmo") - self:get("PrimaryAmmo"), Inventory:get("Bullets9mm"))

        Inventory:Add("Bullets9mm", -Amount1)
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") + Amount1)

        local Amount2 = math.min(self:get("MaxSecondaryAmmo") - self:get("SecondaryAmmo"), Inventory:get("ARGrenades"))

        Inventory:Add("ARGrenades", -Amount2)
        self:set("SecondaryAmmo", self:get("SecondaryAmmo") + Amount2)

        Model:set("Animation", ANIM_LONGIDLE)
        return
    end

    if SequNr == ANIM_GRENADE then
        if self:get("SecondaryAmmo") < 1 and self:CanReload() then
            Model:set("Animation", ANIM_RELOAD)
        else
            Model:set("Animation", ANIM_LONGIDLE)
        end
        return
    end

    if SequNr >= ANIM_SHOOT1 then   -- Also handles ANIM_SHOOT2 and ANIM_SHOOT3.
        if self:get("PrimaryAmmo") < 1 and self:CanReload() then
            Model:set("Animation", ANIM_RELOAD)
        else
            Model:set("Animation", ANIM_LONGIDLE)
        end
        return
    end

    if SequNr == ANIM_LONGIDLE then
        if HumanPlayer:GetRandom(2) == 0 then
            Model:set("Animation", ANIM_IDLE1)
        end
        return
    end

    if SequNr == ANIM_IDLE1 then
        -- Never play the "IDLE1" animation repeatedly.
        Model:set("Animation", ANIM_LONGIDLE)
        return
    end
end


function AR:IsIdle()
    UpdateChildComponents()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_LONGIDLE or SequNr == ANIM_IDLE1
end


function AR:Draw()
    UpdateChildComponents()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function AR:Holster()
    -- Unfortunately, the AR model does not support holstering (it has no "holster" sequence).
    -- UpdateChildComponents()
    -- Model1stPerson:set("Animation", ANIM_HOLSTER)
    return false
end


function AR:CanReload()
    return (self:get("PrimaryAmmo") < self:get("MaxPrimaryAmmo") and Inventory:get("Bullets9mm") > 0) or
           (self:get("SecondaryAmmo") < self:get("MaxSecondaryAmmo") and Inventory:get("ARGrenades") > 0)
end


function AR:FirePrimary()
    if not self:IsIdle() then return end

    if self:get("PrimaryAmmo") < 1 then
        if self:CanReload() then
            Model1stPerson:set("Animation", ANIM_RELOAD)
        end
    else
        self:set("PrimaryAmmo", self:get("PrimaryAmmo") - 1)

        Model1stPerson:set("Animation", ANIM_SHOOT1)
    end

    -- TODO: send primary fire event
    -- TODO: inflict damage
end


function AR:FireSecondary()
    if not self:IsIdle() then return end

    if self:get("SecondaryAmmo") < 1 then
        if self:CanReload() then
            Model1stPerson:set("Animation", ANIM_RELOAD)
        end
    else
        self:set("SecondaryAmmo", self:get("SecondaryAmmo") - 1)

        Model1stPerson:set("Animation", ANIM_GRENADE)
    end

    -- TODO: send secondary fire event
    -- TODO: inflict damage
end


function AR:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Assault Rifle earlier, and our inventory already has the
        -- maximum number of 9mm bullets and AR grenades, don't pick up anything.
        if Inventory:CheckMax("Bullets9mm") and Inventory:CheckMax("ARGrenades") then
            return false
        end

        Inventory:Add("Bullets9mm", 2 * self:get("MaxPrimaryAmmo"))
        Inventory:Add("ARGrenades", 2 * self:get("MaxSecondaryAmmo"))
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo",   self:get("MaxPrimaryAmmo"))
        self:set("SecondaryAmmo", self:get("MaxSecondaryAmmo"))

        Inventory:Add("Bullets9mm", self:get("MaxPrimaryAmmo"))
        Inventory:Add("ARGrenades", self:get("MaxSecondaryAmmo"))

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary   ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("secondary ammo: " .. self:get("SecondaryAmmo") .. "\n")
    -- Console.Print("inv Bullets9mm:   " .. Inventory:get("Bullets9mm") .. "\n")
    -- Console.Print("inv ARGrenades:   " .. Inventory:get("ARGrenades") .. "\n")
    return true
end


function AR:GetCrosshairInfo()
    return "Gui/CrossHair1"
end


function AR:GetAmmoString()
    return string.format("Ammo %2u (%2u) | %2u (%2u)",
        self:get("PrimaryAmmo"), Inventory:get("Bullets9mm"),
        self:get("SecondaryAmmo"), Inventory:get("ARGrenades"))
end
