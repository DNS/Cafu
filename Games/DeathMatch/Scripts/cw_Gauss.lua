local Gauss          = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = Gauss:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE    = 0
local ANIM_IDLE2   = 1
local ANIM_FIDGET  = 2
local ANIM_SPINUP  = 3
local ANIM_SPIN    = 4
local ANIM_FIRE    = 5
local ANIM_FIRE2   = 6
local ANIM_HOLSTER = 7
local ANIM_DRAW    = 8


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
        Console.Print("Gauss DRAW sequence wrapped, switching to idle.\n")
        Model:set("Animation", ANIM_IDLE)
    elseif SequNr == ANIM_IDLE then
        Console.Print("Gauss IDLE sequence wrapped.\n")
    elseif SequNr == ANIM_HOLSTER then
        Console.Print("Gauss HOLSTER sequence wrapped, selecting next weapon.\n")
        HumanPlayer:SelectNextWeapon()
    end
end


function Gauss:IsIdle()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    return Sequ == ANIM_IDLE or Sequ == ANIM_IDLE2 or Sequ == ANIM_FIDGET
end


function Gauss:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function Gauss:Holster()
    Update1stPersonModel()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function Gauss:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the Gauss gun earlier, and our inventory already has the
        -- maximum number of cells, don't pick up anything.
        if Inventory:CheckMax("Cells") then
            return false
        end

        Inventory:Add("Cells", 2 * self:get("MaxPrimaryAmmo"))
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))

        Inventory:Add("Cells", self:get("MaxPrimaryAmmo"))

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv Cells:   " .. Inventory:get("Cells") .. "\n")
    return true
end
