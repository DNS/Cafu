local PlayerScript   = ...  -- Retrieve the ComponentScriptT instance that is responsible for this script.
local Entity         = PlayerScript:GetEntity()
local PlayerData     = Entity:GetComponent("HumanPlayer")
local Trafo          = Entity:GetTransform()
local Model3rdPerson = Entity:GetComponent("Model")
local PlPhysics      = Entity:GetComponent("PlayerPhysics")
local Inventory      = Entity:GetComponent("Inventory")

-- These constants for PlayerData:get("State") mirror those defined in CompHumanPlayer.cpp.
local STATE_ALIVE            = 0
local STATE_DEAD             = 1
local STATE_FROZEN_SPECTATOR = 2
local STATE_FREE_SPECTATOR   = 3

PlayerScript.EVENT_TYPE_PRIMARY_FIRE   = 1
PlayerScript.EVENT_TYPE_SECONDARY_FIRE = 2

PlayerScript:InitEventTypes(2)


Inventory:set("MaxBullets9mm",   250)
Inventory:set("MaxBullets357",    36)
Inventory:set("MaxShells",       125)
Inventory:set("MaxARGrenades",     4)
Inventory:set("MaxHandGrenades",   7)
Inventory:set("MaxRockets",        5)
Inventory:set("MaxArrows",        30)
Inventory:set("MaxCells",        200)
Inventory:set("MaxFaceHuggers",    5)


function PlayerScript:AddFrag(NumFrags)
    local AmmoArray = PlayerData:get("HaveAmmo")

    -- This is where the frags count is stored...
    AmmoArray[16] = AmmoArray[16] + NumFrags

    PlayerData:set("HaveAmmo", AmmoArray)
end


function PlayerScript:TakeDamage(OtherEnt, Amount, ImpDirX, ImpDirY, ImpDirZ)
    -- Only human players that are still alive can take damage.
    if PlayerData:get("State") ~= STATE_ALIVE or PlayerData:get("Health") <= 0 then
        return
    end

    -- Nudge the entity into the direction of ImpDir.
    local vx, vy, vz = PlPhysics:get("Velocity")

    vx = vx + 20.0 * Amount * ImpDirX
    vy = vy + 20.0 * Amount * ImpDirY

    PlPhysics:set("Velocity", vx, vy, vz)

    -- Reduce the entity's health.
    if Amount < PlayerData:get("Health") then
        PlayerData:set("Health", PlayerData:get("Health") - Amount)

        Console.Print("Something hurt the player, new health is " .. tostring(PlayerData:get("Health")) .. "\n")
        return
    end

    -- The player was killed.
    Console.Print("The player was killed.\n")

    PlayerData:set("State", STATE_DEAD)
    PlayerData:set("Health", 0)

    -- Now that the player is dead, clear the collision model.
    Entity:GetComponent("CollisionModel"):set("Name", "")

    -- Update the 3rd person model.
    local fx, fy, fz = Trafo:GetAxisX()     -- "forward" axis
    local lx, ly, lz = Trafo:GetAxisY()     -- "left" axis

    local d1 = fx*ImpDirX + fy*ImpDirY + fz*ImpDirZ
    local d2 = lx*ImpDirX + ly*ImpDirY + lz*ImpDirZ

    if d1 > 0.7 then
        Model3rdPerson:set("Animation", 21)                     -- die forward (315° ...  45°)
    elseif d1 > 0.0 then
        Model3rdPerson:set("Animation", d2 > 0 and 23 or 22)    -- die spin (270° ... 315°), headshot (45° ...  90°)
    elseif d1 > -0.7 then
        Model3rdPerson:set("Animation", d2 > 0 and 18 or 24)    -- die simple (225° ... 270°), gutshot (90° ... 135°)
    else
        Model3rdPerson:set("Animation", d2 > 0 and 20 or 19)    -- die backwards (180° ... 225°), die backwards1 (135° ... 180°)
    end

    -- Count the frag at the entity that originally caused us the damage.
    --
    -- TODO: What if OtherEnt is a Grenade, Rocket, etc.?
    --       AddFrag() should be called for the player who fired it, not for the Grenade itself.
    --       Answer: The Grenade should probably keep a reference to its originating player,
    --       and have a method that we can use to query it.
    --
    local OtherScript = OtherEnt:GetComponent("Script")

    if OtherScript and OtherScript.AddFrag then
        OtherScript:AddFrag(OtherEnt:GetID() == Entity:GetID() and -1 or 1)
    end
end


-- This method is called automatically on the client whenever an event arrives.
function PlayerScript:ProcessEvent(EventType, NumEvents)
    -- The handling of player events is implemented in the "HumanPlayer"
    -- component at this time, so forward all events there.
    PlayerData:ProcessEvent(EventType, NumEvents)
end


--[[
This method is called automatically when some other entity wants us to pick up some item.
As a technical detail, here is a typical call-chain that can cause calls to this method:

  - An entity (a CollisionModel component) finds itself in another entity's trigger volume.
  - The other entity's `OnTrigger()` script callback is called.
  - If the other entity is a weapon, its `OnTrigger()` implementation in `Weapon.lua`
    calls the original (this) entity's `PickUpItem()` script method.
--]]
function PlayerScript:PickUpItem(ItemEnt, ItemType)
    -- The details of picking up items are implemented in the "HumanPlayer"
    -- component at this time, so forward all calls there.
    PlayerData:PickUpItem(ItemType)

    -- Find the CarriedWeapon component that matches the picked up item,
    -- then let it know that the item can be picked up.
    for i = 0, 32 do
        local cw = Entity:GetComponent("CarriedWeapon", i)

        if not cw then break end

        if cw:get("Label") == ItemType and cw.PickedUp then
            if cw:PickedUp() then
                print("Player picked up item:", ItemType)
                return true
            end

            return false
        end
    end

    -- No carried weapon handled the item.
    -- Now see if we can put it into the Inventory.
    if ItemType == "Ammo_DartGun" then
        Inventory:Add("Arrows", 5)
        return true
    end

    if ItemType == "Ammo_DesertEagle" then
        Inventory:Add("Bullets357", 6)
        return true
    end

    if ItemType == "Ammo_Gauss" then
        Inventory:Add("Cells", 20)
        return true
    end

    return false
end


function Model3rdPerson:OnAnimationChange(AnimNr)
    if (AnimNr < 18) or (AnimNr > 24) then
        -- The player is "alive", so blend animation sequences in 0.3 seconds,
        -- and force their playback in a loop.
        -- Console.Print("[alive] OnAnimationChange " .. AnimNr .. "\n")
        return 0.3, true
    end

    -- TODO: It would be nice if we also had the number of frame and FPS of the animation sequence,
    -- and could then use   math.min(0.2, (NumFrames - 1) * FPS * 0.5)   as blend time.
    -- Console.Print("[dead] OnAnimationChange " .. AnimNr .. "\n")
    return 0.2, false
end
