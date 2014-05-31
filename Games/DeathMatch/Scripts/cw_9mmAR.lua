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


local function Update1stPersonModel()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:GetChildren()[2]:GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Console.Print("Assault Rifle DRAW sequence wrapped, switching to idle.\n")
        Model:set("Animation", ANIM_IDLE1)
    elseif SequNr == ANIM_IDLE1 then
        Console.Print("Assault Rifle IDLE1 sequence wrapped.\n")
 -- elseif SequNr == ANIM_HOLSTER then
 --     HumanPlayer:SelectNextWeapon()
    end
end


function AR:IsIdle()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    return Sequ == ANIM_LONGIDLE or Sequ == ANIM_IDLE1
end


function AR:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function AR:Holster()
    -- Unfortunately, the AR model does not support holstering (it has no "holster" sequence).
    -- Update1stPersonModel()
    -- Model1stPerson:set("Animation", ANIM_HOLSTER)
    return false
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
