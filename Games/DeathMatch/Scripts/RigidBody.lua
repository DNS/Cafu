dofile("Games/DeathMatch/Scripts/Vector3.lua")

local RigidBody = ...   -- Retrieve the ComponentScriptT instance that is responsible for this script.
local Physics   = RigidBody:GetEntity():GetComponent("Physics")
local Trafo     = RigidBody:GetEntity():GetTransform()

Trafo:InitClientApprox("Origin")
Trafo:InitClientApprox("Orientation")


-- This callback provides a default behaviour for rigid bodies that are hit by
-- shots, explosions, or other kind of damage. Feel free to copy or vary!  :-)
function RigidBody:TakeDamage(OtherEnt, Amount, ImpDirX, ImpDirY, ImpDirZ)

    -- Just an experimentally determined number that seems to work well.
    local Scale = 2.0

    -- Imp has no meaningful unit...
    local Imp = Vector3T(ImpDirX, ImpDirY, ImpDirZ) * (Scale * Amount)

    -- Assumes that the damage was caused / originated at OtherEnt's origin...
    local Ob = Vector3T(OtherEnt:GetTransform():get("Origin"))

    -- ... and is directed towards our origin.
    local Oc = Vector3T(Trafo:get("Origin"))

    -- See http://www.bulletphysics.com/Bullet/phpBB3/viewtopic.php?f=9&t=3079
    -- for more details, especially for why we can compute `rel_pos` correctly
    -- as `Ob - Oc`, instead of having to compute the exact location of the impact!
    local rel_pos = Ob - Oc

    -- Use our entity's physics component to apply the impulse.
    Physics:ApplyImpulse(Imp, rel_pos)
end
