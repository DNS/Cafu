local FaceHugger     = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = FaceHugger:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Inventory      = Entity:GetComponent("Inventory")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE      = 0
local ANIM_FIDGETFIT = 1
local ANIM_FIDGETNIP = 2
local ANIM_HOLSTER   = 3
local ANIM_DRAW      = 4
local ANIM_THROW     = 5


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
        Console.Print("FaceHugger DRAW sequence wrapped, switching to idle.\n")
        Model:set("Animation", ANIM_IDLE)
    elseif SequNr == ANIM_IDLE then
        Console.Print("FaceHugger IDLE sequence wrapped.\n")
    elseif SequNr == ANIM_HOLSTER then
        Console.Print("FaceHugger HOLSTER sequence wrapped, selecting next weapon.\n")
        HumanPlayer:SelectNextWeapon()
    end
end


function FaceHugger:IsIdle()
    Update1stPersonModel()

    local Sequ = Model1stPerson:get("Animation")

    return Sequ == ANIM_IDLE or Sequ == ANIM_FIDGETFIT or Sequ == ANIM_FIDGETNIP
end


function FaceHugger:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function FaceHugger:Holster()
    Update1stPersonModel()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
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

        -- Inventory:Add("FaceHuggers", 1)

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    -- Console.Print("inv FaceHuggers:   " .. Inventory:get("FaceHuggers") .. "\n")
    return true
end
