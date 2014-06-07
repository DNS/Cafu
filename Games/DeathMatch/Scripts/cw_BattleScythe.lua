local BattleScythe   = ...  -- Retrieve the ComponentCarriedWeaponT instance that is responsible for this script.
local Entity         = BattleScythe:GetEntity()
local HumanPlayer    = Entity:GetComponent("HumanPlayer")
local Model1stPerson = nil


-- Symbolic names for the animation sequences of the 1st-person weapon model.
local ANIM_IDLE        =  0
local ANIM_DRAW        =  1
local ANIM_HOLSTER     =  2
local ANIM_ATTACK1     =  3
local ANIM_ATTACK1MISS =  4
local ANIM_ATTACK2     =  5
local ANIM_ATTACK2HIT  =  6
local ANIM_ATTACK3     =  7
local ANIM_ATTACK3HIT  =  8
local ANIM_IDLE2       =  9   -- same as ANIM_IDLE
local ANIM_IDLE3       = 10   -- same as ANIM_IDLE


local function Update1stPersonModel()
    -- On the client, our entity's *children* may arrive over the network only *after*
    -- this script has initially been run (and this component's OnInit() has been called).
    -- Therefore, we have to defer the Model1stPerson init until it is first used.
    if not Model1stPerson then
        Model1stPerson = Entity:GetChildren()[2]:GetComponent("Model")
    end
end


local function OnSequenceWrap_Sv(Model)     -- Model == Model1stPerson as assigned in Draw() below.
    local self   = BattleScythe
    local SequNr = Model:get("Animation")

    if SequNr == ANIM_DRAW then
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_HOLSTER then
        HumanPlayer:SelectNextWeapon()
        return
    end

    if SequNr >= ANIM_ATTACK1 and SequNr <= ANIM_ATTACK3HIT then
        Model:set("Animation", ANIM_IDLE)
        return
    end

    if SequNr == ANIM_IDLE then
        return
    end
end


function BattleScythe:IsIdle()
    Update1stPersonModel()

    local SequNr = Model1stPerson:get("Animation")

    return SequNr == ANIM_IDLE or SequNr == ANIM_IDLE2 or SequNr == ANIM_IDLE3
end


function BattleScythe:Draw()
    Update1stPersonModel()

    Model1stPerson:set("Show", true)
    Model1stPerson:set("Name", self:get("Model1stPerson"))
    Model1stPerson:set("Animation", ANIM_DRAW)
    Model1stPerson.OnSequenceWrap_Sv = OnSequenceWrap_Sv
end


function BattleScythe:Holster()
    Update1stPersonModel()

    Model1stPerson:set("Animation", ANIM_HOLSTER)
    return true
end


function BattleScythe:FirePrimary()
    if not self:IsIdle() then return end

    if HumanPlayer:GetRandom(2) == 0 then
        Model1stPerson:set("Animation", ANIM_ATTACK1MISS)
    else
        Model1stPerson:set("Animation", ANIM_ATTACK3)
    end

    -- TODO: send primary fire event
    -- TODO: inflict damage
end


function BattleScythe:FireSecondary()
    if not self:IsIdle() then return end

    Model1stPerson:set("Animation", ANIM_ATTACK2)

    -- TODO: send secondary fire event
    -- TODO: inflict damage
end


function BattleScythe:PickedUp()
    if self:get("IsAvail") then
        -- If we have picked up the BattleScythe earlier, don't pick up anything.
        return false
    else
        self:set("IsAvail", true)
        self:set("PrimaryAmmo", self:get("MaxPrimaryAmmo"))   -- This is actually 0.

        HumanPlayer:SelectWeapon(self)
    end

    -- Console.Print("primary Ammo: " .. self:get("PrimaryAmmo") .. "\n")
    return true
end
