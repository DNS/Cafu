local EntFac = ...  -- Retrieve the ComponentScriptT instance that is responsible for this script.
local Trafo  = EntFac:GetEntity():GetTransform()

EntFac.TimeSinceLast = 0.0


function EntFac:Think(FrameTime)
    local Parent = self:GetEntity():GetParent()

    -- Newly created entities are created as our siblings, so that the transform
    -- components are in the same coordinate space. Therefore, if for some reason
    -- we have no parent, don't do anything.
    if not Parent then return end

    -- If we have created all children, create no more.
    -- Note that `self.NumTotal == -1` means "unlimited".
    if self.NumTotal == 0 then return end

    -- If not enough time has passed since we created the last entity, wait.
    self.TimeSinceLast = self.TimeSinceLast + FrameTime
    if self.TimeSinceLast < self.Delay then return end

    -- If some other entity is closer than `self.Clearance`, wait.
    ;   -- TODO

    -- Ready to create the new entity.
    local NewEnt = world:new("EntityT", self.Type)

    Parent:AddChild(NewEnt)
    NewEnt:GetTransform():set("Origin", Trafo:get("Origin"))
    NewEnt:GetTransform():set("Orientation", Trafo:get("Orientation"))

    if self.Type == "CompanyBot" then
        local c1 = world:new("ComponentModelT")
        c1:set("Show", true)
        c1:set("Name", "Games/DeathMatch/Models/Players/Trinity/Trinity.cmdl")
        c1:set("Animation", 0)
        c1:set("Skin", -1)
        c1:set("Scale", 1)
        c1:set("Gui", "")
        c1:set("IsSubmodel", false)

        local c2 = world:new("ComponentModelT")
        c2:set("Show", true)
        c2:set("Name", "Games/DeathMatch/Models/Weapons/DesertEagle/DesertEagle_p.cmdl")
        c2:set("Animation", 0)
        c2:set("Skin", -1)
        c2:set("Scale", 1)
        c2:set("Gui", "")
        c2:set("IsSubmodel", true)

        local c3 = world:new("ComponentPlayerPhysicsT")
        c3:set("Velocity", 0, 0, 0)
        c3:set("Dimensions", -12, -12, -36, 12, 12, 36)
        c3:set("StepHeight", 24)

        local c4 = world:new("ComponentScriptT")
        c4:set("Name", "Games/DeathMatch/Scripts/CompanyBot.lua")
        c4:set("ScriptCode", "")

        NewEnt:AddComponent(c1, c2, c3, c4)
    elseif self.Type == "Eagle" then
        local c1 = world:new("ComponentModelT")
        c1:set("Show", true)
        c1:set("Name", "Games/DeathMatch/Models/LifeForms/Eagle/Eagle.cmdl")
        c1:set("Animation", 0)
        c1:set("Skin", -1)
        c1:set("Scale", 1)
        c1:set("Gui", "")
        c1:set("IsSubmodel", false)

        local c2 = world:new("ComponentScriptT")
        c2:set("Name", "Games/DeathMatch/Scripts/Eagle.lua")
        c2:set("ScriptCode", "")

        local c3 = world:new("ComponentSoundT")
        c3:set("Name", "Ambient/Jungle")
        c3:set("AutoPlay", true)
        c3:set("Interval", 20)

        NewEnt:AddComponent(c1, c2, c3)
    else  -- self.Type == "Butterfly"
        local c1 = world:new("ComponentModelT")
        c1:set("Show", true)
        c1:set("Name", "Games/DeathMatch/Models/LifeForms/Butterfly/Butterfly.cmdl")
        c1:set("Animation", 0)
        c1:set("Skin", -1)
        c1:set("Scale", 1)
        c1:set("Gui", "")
        c1:set("IsSubmodel", false)

        local c2 = world:new("ComponentScriptT")
        c2:set("Name", "Games/DeathMatch/Scripts/Butterfly.lua")
        c2:set("ScriptCode", "")

        NewEnt:AddComponent(c1, c2)
    end

    Console.Print("Entity of type " .. self.Type .. " created.\n")

    if self.NumTotal ~= -1 then self.NumTotal = self.NumTotal - 1 end
    self.TimeSinceLast = 0.0
end
