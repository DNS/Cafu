function MyFirstLift:OnInit()
    Console.Print("Hallo aus der MyFirstLift:OnInit() Funktion!\n");
end


MyFirstLift.Count=0;
isMoving=false;
isUp=false;


function MyFirstLift:OnTrigger()
    self.Count=self.Count+1;

--[[    Console.Print("Hallo aus der MyFirstLift:OnTrigger() Funktion, Aufruf " .. self.Count .. ".\n");

    local x, y, z=self:GetOrigin();
    Console.Print("Origin alt: " .. x .. " " .. y .. " " .. z .. "\n");

    self:SetOrigin(x, y, z+500);

    x, y, z=self:GetOrigin();
    Console.Print("Origin neu: " .. x .. " " .. y .. " " .. z .. "\n\n");

    -- ci.RunCommand("list();");
--]]

    local variant=2;

    if (variant==1) then
        if (isMoving) then return end

        isMoving=true;
        local x, y, z=self:GetOrigin();
        self:Translate(x, y, z+2000, 1.5);
        coroutine.yield(2.5);
        self:Translate(x, y, z,      1.5);
        coroutine.yield(1.5);
        isMoving=false;
    else
        if (isMoving) then return end

        isMoving=true;
        local x, y, z=self:GetOrigin();

        if (isUp) then self:Translate(x, y, z-3000, 1.5);
                  else self:Translate(x, y, z+3000, 1.5); end
        coroutine.yield(1.5);

        isUp=not isUp;
        isMoving=false;
    end
end


-- function MyFirstLift:Serialize()
-- end

-- function MyFirstLift:Deserialize()
-- end

-- Console.Print("Hallo aus dem .lua File!\n");


function LightBlinkThread(lightEntity)
    -- Notice the great generality of this function:
    -- It can be used with *any* light entity, which is passed in via the lightEntity parameter!
    while (true) do
        lightEntity:SetColor(0, 255, 0);
        coroutine.yield(0.5);             -- Wait 0.5 seconds.
        if (not lightEntity.IsBlinking) then break; end

        lightEntity:SetColor(255, 0, 0);
        coroutine.yield(0.5);             -- Wait 0.5 seconds.
        if (not lightEntity.IsBlinking) then break; end
    end
end


MyLight.IsRed=false;
MyLight.IsBlinking=false;


function MyLight:OnTrigger()
    -- Choose the variant of logic you want to test.
    local variant=3;

    -- The logic is here and not in the GUI script because the GUI script code is also run on client prediction.
    -- This map script code is only run on server thinking.
    if (variant==1) then
        -- Implements a toggle switch.
        if (self.IsRed) then
            self:SetColor(0, 255, 0);
        else
            self:SetColor(255, 0, 0);
        end

        self.IsRed=not self.IsRed;
    elseif (variant==2) then
        -- Toggles between permanent red/green blinking and not blinking.
        if (self.IsBlinking) then
            -- Currently blinking, turn it off now.
            -- This will break the infinite loop below.
            self.IsBlinking=false;
            self:SetColor(oldR, oldG, oldB);
        else
            -- Currently not blinking, turn it on now.
            oldR, oldG, oldB=self:GetColor();
            self.IsBlinking=true;
            while (true) do
                self:SetColor(0, 255, 0);
                coroutine.yield(0.5);             -- Wait 0.5 seconds.
                if (not self.IsBlinking) then break; end

                self:SetColor(255, 0, 0);
                coroutine.yield(0.5);             -- Wait 0.5 seconds.
                if (not self.IsBlinking) then break; end
            end
        end
    elseif (variant==3) then
        -- As variant 2 above (toggles between permanent red/green blinking and not blinking),
        -- but implemented with an explicit thread (coroutine).
        -- This is the Ca3DE equivalent/analog to the Doom3 thread example in base\script\map_marscity2.script,
        -- but more elaborate and versatile (the LightBlinkThread function is reusable with any light entity).
        if (self.IsBlinking) then
            -- Currently blinking, turn it off now.
            -- This will break the infinite loop in the thread.
            self.IsBlinking=false;
            self:SetColor(oldR, oldG, oldB);
        else
            -- Currently not blinking, turn it on now.
            oldR, oldG, oldB=self:GetColor();
            self.IsBlinking=true;

            -- Register the function LightBlinkThread as a new thread. Call with "self" as the first parameter.
            thread(LightBlinkThread, self);
        end
    else
        -- Implements a one-time blink.
        local r, g, b=self:GetColor();  -- Save the previous color.

        self:SetColor(255, 255, 0);     -- Set to yellow.
        coroutine.yield(1);             -- Wait 1 sec.
        self:SetColor(r, g, b);         -- Set back previous color.
    end
end


TestTrigger.Count=0;
TestTrigger.IsBusy=false;

function TestTrigger:OnTrigger(Activator)
    -- Choose the variant of logic you want to test.
    local variant=2;

    -- if (self.Count==0) then
    --     self:Activate(nil);   Console.Print("09 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self:Activate();      Console.Print("10 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self:Activate(false); Console.Print("11 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self:Activate(true);  Console.Print("12 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self:Activate(0);     Console.Print("13 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self:Activate(1);     Console.Print("14 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self:Activate("0");   Console.Print("15 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self:Activate("2");   Console.Print("16 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self:Activate(self);  Console.Print("17 IsActive: " .. tostring(self:IsActive()) .. "\n");
    --     self.Count=self.Count+1;
    -- end

    if (variant==1) then
        -- Note that we do not allow re-entrancy while we are busy.
        if (self.IsBusy) then return; end
        self.IsBusy=true;

        Console.Print("[A] This is run only every 3 seconds.\n");
        coroutine.yield(3);             -- Wait 3 secs.

        self.IsBusy=false;
    elseif (variant==2) then
        -- This is a more elegant (and a bit more efficient) version of variant 1.
        self:Deactivate();      -- No more calls to this function please!

        Console.Print("[B] This is run only every 3 seconds.\n");
        -- MyFirstLift:OnTrigger();
        coroutine.yield(3);             -- Wait 3 secs.

        self:Activate();        -- Ok, engine may call this function again.
    else
        Console.Print("Hi, this is entity " .. self:GetName() .. ". ");
        Console.Print("I was triggered by entity " .. Activator:GetName() .. ", Count " .. self.Count .. "\n");
        self.Count=self.Count+1;
    end
end


SoundTrigger.IsActive=false;

function SoundTrigger:OnTrigger(Activator)
    self:Deactivate();      -- No more calls to this function please!

    if (self.IsActive==false) then
        self.IsActive=true;
        TestSpeaker:Play();
    else
        self.IsActive=false;
        TestSpeaker:Stop();
    end

    coroutine.yield(10);

    self:Activate();        -- Ok, engine may call this function again.
end
