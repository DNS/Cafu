print=function(s)
    Console.Print(tostring(s) .. "\n");
end;

exit=function()
    quit=true;
end

-- Just for debugging the new physics code...
function ph(grav)
    grav=grav or -9.81;
    runMapCmd("wait(3); crate_001:SetGravity(0, 0, "..grav..");");
end

-- Load the console variables with persistent values.
do
    CleanupPersistentConfig("config_p.lua");
    local Result, ErrorMsg=loadfile("config_p.lua");

    if (Result) then
        Result();
    else
        print("\nWarning: Error when running config_p.lua ("..ErrorMsg..").\n");
    end
end


-- Get the (sorted) list of files available as level intro music.
MusicFiles=Console.GetDir("Games/DeathMatch/Music", "f");
table.sort(MusicFiles, function (s1, s2) return s1:lower()<s2:lower(); end);  -- Sort the titles regardless of case.

-- Filter the list (not every file is a good candidate for level intro music) and add the full path.
LevelIntroTitles={};
for FileNr, FileName in ipairs(MusicFiles) do
    if ((FileName:sub(-4, -1)==".ogg" or FileName:sub(-4, -1)==".mp3") and FileName:find("Franka Jones, Track3")==nil) then
        LevelIntroTitles[#LevelIntroTitles+1]="Games/DeathMatch/Music/"..FileName;
    end
end

-- The client calls this function whenever the player enters a new level.
function StartLevelIntroMusic()
    if (#LevelIntroTitles==0) then return end;

    local NextTitleNr=0;

    -- Read the index number of the next title to play.
    local File=io.open("Games/DeathMatch/Music/NextTitle.txt", "rt");

    if (File) then
        NextTitleNr=(File:read("*number") or 0) % #LevelIntroTitles;
        File:close();
    end

    -- Update the next index number count.
    local File=io.open("Games/DeathMatch/Music/NextTitle.txt", "wt");

    if (File) then
        File:write((NextTitleNr+1) % #LevelIntroTitles);
        File:close();
    end

    MusicLoad(LevelIntroTitles[NextTitleNr+1]);     -- First array index is 1, not 0.
    MusicSetVolume(0.5);
    MusicPlay();
end


-- sv_AutoAddCompanyBot=true;
-- cl_maxLights=20;


sv_rc_password="ca3de";     -- Change this for your own (dedicated) servers!
cl_rc_password="ca3de";


print("config.lua processed.");
