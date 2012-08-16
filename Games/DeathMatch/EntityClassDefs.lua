-- Let DeathMatch maps be up to 131072 (65536*2) units big in each direction. This corresponds to a side length of 3329,23 km.
-- Note: If this gets any bigger than 65536, CaWE needs revision, especially cmap brush loading (starting digging from LoadSave_cmap.cpp)!
Mapsize={ -65536, 65536 }


-- Returns a clone of the given argument "node".
-- This is especially interesting when node is a table, because then a deep copy is made.
function clone(node)
    if type(node)~="table" then return node end

    -- node is a table, make a deep copy.
    local b={};
    table.foreach(node, function(k,v) b[k]=clone(v) end);

    return b;
end


-- This function takes a variable number of arguments, all of which must be of type "table".
-- It deep-copies all tables into one common table, and returns the combined result.
function newEntClassDef(...)
    local ResultTable={};

    for i, SubTable in ipairs{...} do
        -- Put a clone of each element of SubTable into ResultTable.
        for k, v in pairs(SubTable) do
            ResultTable[k]=clone(v);
        end
    end

    return ResultTable;
end


-- A base class with common definitions.
Common=
{
    name=
    {
        type       ="string";
        description="The name of this entity. It must be unique and non-empty.";
     -- editor_useredit=true;
        uniqueValue=true;
    };

    -- origin={ ... };
}


-- A base class for orientable entities.
Angles=
{
    angles=
    {
        type       ="string";
        description="Bank Heading Pitch (Y Z X)";
        value      ="0 0 0";
    };
}


-- This table contains the definitions of all entity classes in the game.
-- * Connection to C++ classes (CppClass key).
-- * Defines supported variables (properties, key/value pairs) by that (C++) entity class.
-- * Default values for instantiated entities.
-- * Auxiliary information for the Ca3DE World Editor CaWE.
EntityClassDefs={}


EntityClassDefs["worldspawn"]=newEntClassDef(Common,
{
    isSolid    =true;
    CppClass   ="EntWorldspawnT";
    description="The world entity.";

    message=
    {
        type       ="string";
        description="This value is shown when the map starts.";
     -- editor_useredit=true;
    };

    lightmap_patchsize=
    {
        type       ="string";
        description="LightMap PatchSize";
        value      ="200.0";
    };

    shlmap_patchsize=
    {
        type       ="string";
        description="SHL Map PatchSize";
        value      ="200.0";
    };
})


EntityClassDefs["HumanPlayer"]=newEntClassDef(Common,
{
    isPoint    =true;
    CppClass   ="EntHumanPlayerT";
    description="A human player entity. Should never be placed in a map or even shown in the editor, it just doesn't make sense.";
})


EntityClassDefs["corpse"]=newEntClassDef(Common,
{
    isPoint    =true;
    CppClass   ="EntCorpseT";
    description="A corpse entity. Usually created by code.";
})


-- This is the "complement" to the texture area lights.
EntityClassDefs["PointLight"]=newEntClassDef(Common, Angles,
{
    isPoint    =true;
 -- CppClass   =...;    -- Cannot instantiate this in a map, it's for CaLight only.
    description="Radiosity Point Light";
 -- iconsprite("sprites/lightbulb.spr")

    opening_angle=
    {
        type       ="integer";
        description="Opening angle [0..180]";
        value      =180;
    };

    intensity_r=
    {
        type       ="integer";
        description="Intensity [W/sr] red";
    };

    intensity_g=
    {
        type       ="integer";
        description="Intensity [W/sr] green";
    };

    intensity_b=
    {
        type       ="integer";
        description="Intensity [W/sr] blue";
    };
})


-- An entity for a dynamic light source (like Doom3 lights).
EntityClassDefs["PointLightSource"]=newEntClassDef(Common, Angles,
{
    isPoint    =true;
    CppClass   ="EntPointLightSourceT";
    description="Dynamic Light Source";
 -- iconsprite("sprites/lightbulb.spr")

    light_radius=
    {
        type       ="integer";
        description="Dyn. light radius";
        value      ="500";
    };

    light_color_diff=
    {
        type       ="color";
        description="Dyn. light diffuse color";
        value      ="255 255 128";      -- Provide users with a reasonable default (warm yellow-white).
    };

    light_color_spec=
    {
        type       ="color";
        description="Dyn. light specular color";
        value      ="255 255 128";      -- Provide users with a reasonable default (warm yellow-white).
    };

    light_casts_shadows=
    {
        type       ="flags";    -- This should actually be "bool".
        description="Flags about whether the light casts shadows.";
        flags      ={ "Light casts shadows", true };
    };
})


-- Rename this to "generic"?   "A generic entity with no special purposes."
EntityClassDefs["func_wall"]=newEntClassDef(Common,
{
    isSolid    =true;
    description="A wall.";
})


EntityClassDefs["info_generic"]=newEntClassDef(Common, Angles,
{
    isPoint    =true;
    CppClass   ="EntInfoGenericT";
    description="A generic entity that provides additional information at any given point in the map.\n" ..
                "It's main purpose is to mark points in the map that the map scripts can refer to.";
    size       ={ { -16, -16, -16 }, { 16, 16, 16 } };
    color      ={ 255, 255, 200 };      -- Very bright yellow.

    param1     ={ type="string"; description="A generic parameter for use by the map script."; };
    param2     ={ type="string"; description="A generic parameter for use by the map script."; };
    param3     ={ type="string"; description="A generic parameter for use by the map script."; };
})


EntityClassDefs["func_water"]=newEntClassDef(Common,
{
    isSolid    =true;
    description="A body of water.";
})


-- Info Player Start.
-- This is not a game-specific entity, but rather an engine entity, simply because it is sensible for each game to have at least one such.
-- Moreover, CaBSP even requires for its proper operation that at least one such entity exists, and that they all are *INSIDE* the actual map!
EntityClassDefs["info_player_start"]=newEntClassDef(Common, Angles,
{
    isPoint    =true;
    CppClass   ="EntInfoPlayerStartT";
    description="A starting location for the player.";
    size       ={ { -16, -16, -36 }, { 16, 16, 36 } };      -- Rename in editor_bbox? Rename color in editor_bbox_color?
    color      ={ 0, 255, 0 };
})


-- Für Leitern, Taue und alles, woran man klettern kann. Dieser Entity ist unsichtbar!
EntityClassDefs["func_ladder"]=newEntClassDef(Common,
{
    isSolid    =true;
    CppClass   ="EntFuncLadderT";
    description="Ladder (for climbing)";
})


-- An entity for general-purpose movements, like doors, lifts, moving platforms, rotating machinery, etc.
EntityClassDefs["func_mover"]=newEntClassDef(Common,
{
    isSolid    =true;
    CppClass   ="EntFuncMoverT";
    description="An entity for moving objects like doors, lifts, platforms, etc. To be used with scripting.";
})


-- This entity class implements "doors".
-- It's a pure convenience class, because the same functionality could be implemented with func_mover entities and scripts.
EntityClassDefs["func_door"]=newEntClassDef(Common,
{
    isSolid    =true;
    CppClass   ="EntFuncDoorT";
    description="An entity that implements usual, automatic doors.";


    -- The following keys are defined essentially in the order of activities that occur
    -- when a player walks to the door, the door opens, waits, and closes again.

    triggerPadding=
    {
        type       ="float";
        description="How near the player must come for the door to open, that is, how far the implicit door trigger extends from the door.";
        value      =60*25.4;
    };

    startOpen=
    {
        type       ="integer";  -- "bool"
        description="When set, the door starts in the open position when spawned. This is useful to temporarily or permanently close off an area when triggered. When you use this, you should also set \"noTouch\" to true and \"openTime\" to -1.";
        value      =false;
    };

    -- mode = trigger, auto, locked, toggle.
    locked=
    {
        type       ="integer";  -- "choice"
        description="used in conjunction with no_touch,the door must be triggered by a trigger, after which it works normally.  if locked = 1 then the door opens when unlocked, if locked = 2 then the door just becomes unlocked.";
        value      =0;
    };

    no_touch=
    {
        type       ="integer";  -- "bool"
        description="the door should not be triggered by the player touching it, only by another trigger. in multiplayer, this door can't be shot to open.";
        value      =false;
    };

    health=
    {
        type       ="integer";
        description="if set, the door must be shot open.";
        value      =0;
    };

    moveDir=
    {
        type       ="string";   -- "direction"
        description="Determines the direction of movement for opening the door.";
        value      ="0 1 0";
    };

    moveTime=
    {
        type       ="float";
        description="How long the open and close movements take, in seconds.";
        value      =5;
    };

    lip=
    {
        type       ="float";
        description="Offset to usual movement distance. Positive values keep the door from disappearing entirely into the wall, negative values move it into the wall even farther.";
        value      =8*25.4;
    };

    toggle=
    {
        type       ="integer";  -- "bool"
        description="wait at each position until triggered again.";
        value      =0;
    };

    openTime=
    {
        type       ="float";
        description="Delay in seconds before the opened door automatically closes again. If -1, the door doesn't close automatically.";
        value      =3;
    };

    crusher=
    {
        type       ="integer";  -- "bool"
        description="Controls the doors behaviour when it is blocked while closing. When set, the door will just halt (and continue trying to close). Otherwise (the default), it will reverse (re-open).";
        value      =false;
    };

    damage=
    {
        type       ="float";
        description="The amount of damage to inflict to an entity that is blocking the door.";
        value      =0;
    };


    -- These keys are either "generic" for the entity or intentionally grouped separately.

    team=
    {
        type       ="string";   -- "entityList"
        description="All doors that have this key set to a common value will act as a team.";
        value      ="";
    };

    buddy=
    {
        type       ="string";   -- "entityList"
        -- Toggles the value of the "shaderparam7" key of the listed entities. Door frames can for example indicate the lock status of the door this way.
        -- Hmmm... can't we somehow generalize this? E.g. have *any* key in the listed entities toggled? Any script function called?
        description="Currently not implemented.";
        value      ="";
    };

    -- It would probably be better to call a user-defined script method instead (e.g. OnClosed())...
    targetsClosed=
    {
        type       ="string";   -- "entityList"
        description="The listed entities are triggered when the door closed.";
        value      ="";
    };

    -- It would probably be better to call a user-defined script method instead (e.g. OnOpened())...
    targetsOpened=
    {
        type       ="string";   -- "entityList"
        description="The listed entities are triggered when the door opened.";
        value      ="";
    };

    -- It would probably be better to call a user-defined script method instead (e.g. OnBlocked())...
    targetsBlocked=
    {
        type       ="string";   -- "entityList"
        description="The listed entities are triggered when the door detects that it is blocked while closing.";
        value      ="";
    };
})


-- The generic "trigger" entity. It's main purpose is to activate (call) script methods whenever something walks into its brushes.
-- (In truth the roles are reversed: It's e.g. the player movement code that checks if it walked itself into a trigger volume, and then
--  calls into the script, so trigger entities are really quite passive. Thinking the other way round is probably more suggestive for users though.)
-- Note that any other generic entity can also have trigger brushes, and thus act as a trigger entity, too.
-- What makes trigger entities special is their unique script methods e.g. for activating or deactivating them
-- (e.g. Activate(), Deactivate(), Reset(), etc.), so the dedicated Trigger entity should be the preferred entity for keeping trigger brushes.
EntityClassDefs["Trigger"]=newEntClassDef(Common,
{
    isSolid    =true;
    CppClass   ="EntTriggerT";
    description="All-round trigger entity for activating (calling) script methods when something enters the trigger volume.";
})


EntityClassDefs["LifeFormMaker"]=newEntClassDef(Common, Angles,
{
    isPoint    =true;
    CppClass   ="EntMonsterMakerT";
    description="This entity creates other entities (\"life forms\").";
    size       ={ { -16, -16, -16 }, { 16, 16, 16 } };

    monstertype=
    {
        type       ="string";
        description="LifeForm Type";
    };

    spawnflags=
    {
        type       ="flags";
        description="Flags that control the spawn state of this entity.";
        flags      ={ "Start ON", false, "Cyclic", false, "MonsterClip", false };
    };

    monstercount=
    {
        type       ="integer";
        description="How many monsters the monstermaker can create (-1 means unlimited).";
        value      =-1;
    };

    delay=
    {
        type       ="string";
        description="How frequently a new monster will be created. If -1, a new monster will only be made when another one dies.";
        value      =-1;
    };

    m_imaxlivechildren=
    {
        type       ="integer";
        description="Maximum number of live children allowed at one time (new ones will not be made until one dies). -1 means no limit.";
        value      =5;
    };
})


-- A base class for weapon entities.
Weapon=newEntClassDef(Common, Angles,
{
    isPoint=true;
    size   ={ { -16, -16, -16 }, { 16, 16, 16 } };
    color  ={ 0, 0, 200 };
})


-- A base class for monster entities.
Monster=newEntClassDef(Common, Angles,
{
    isPoint=true;
    size   ={ { -16, -16, -16 }, { 16, 16, 16 } };
    color  ={ 0, 200, 255 };
})


-- Monsters
EntityClassDefs["monster_argrenade"]=newEntClassDef(Monster, { CppClass="EntARGrenadeT"; description="AR Grenade"; })
EntityClassDefs["monster_butterfly"]=newEntClassDef(Monster, { CppClass="EntButterflyT"; description="Butterfly"; })
EntityClassDefs["monster_companybot"]=newEntClassDef(Monster, { CppClass="EntCompanyBotT"; description="Company Bot (Trinity)"; })
EntityClassDefs["monster_eagle"]=newEntClassDef(Monster, { CppClass="EntEagleT"; description="Eagle"; })
EntityClassDefs["monster_facehugger"]=newEntClassDef(Monster, { CppClass="EntFaceHuggerT"; description="Facehugger"; })
EntityClassDefs["monster_handgrenade"]=newEntClassDef(Monster, { CppClass="EntHandGrenadeT"; description="Handgrenade"; })
EntityClassDefs["monster_rocket"]=newEntClassDef(Monster, { CppClass="EntRocketT"; description="Rocket"; })

-- Ammo
EntityClassDefs["ammo_9mmclip"]=newEntClassDef(Weapon, { CppClass=""; description="9mm Pistol Ammo"; })
EntityClassDefs["ammo_9mmAR"]=newEntClassDef(Weapon, { CppClass=""; description="9mm Assault Rifle Ammo"; })
EntityClassDefs["ammo_9mmbox"]=newEntClassDef(Weapon, { CppClass=""; description="Box of 200 9mm shells"; })
EntityClassDefs["ammo_ARgrenades"]=newEntClassDef(Weapon, { CppClass=""; description="Assault Rifle Grenades"; })
EntityClassDefs["ammo_357"]=newEntClassDef(Weapon, { CppClass="EntItemAmmo357T"; description="357 Pistol Ammo"; })
EntityClassDefs["ammo_shotgun_shells"]=newEntClassDef(Weapon, { CppClass=""; description="Shotgun Shells"; })
EntityClassDefs["ammo_rpg_rocket"]=newEntClassDef(Weapon, { CppClass=""; description="RPG Rockets"; })
EntityClassDefs["ammo_gausscells"]=newEntClassDef(Weapon, { CppClass=""; description="Gauss Gun Cells"; })
EntityClassDefs["ammo_crossbow_arrows"]=newEntClassDef(Weapon, { CppClass="EntItemAmmoArrowT"; description="Crossbow Arrows"; })

-- Weapons
EntityClassDefs["weapon_battlescythe"]=newEntClassDef(Weapon, { CppClass="EntWeaponBattleScytheT"; description="Battle Scythe"; })
EntityClassDefs["weapon_hornetgun"]=newEntClassDef(Weapon, { CppClass="EntWeaponHornetGunT"; description="Hornet Gun"; })
EntityClassDefs["weapon_9mmhandgun"]=newEntClassDef(Weapon, { CppClass="EntWeaponPistolT"; description="9mm Handgun"; })
EntityClassDefs["weapon_357handgun"]=newEntClassDef(Weapon, { CppClass="EntWeapon357T"; description="357 Handgun"; })
EntityClassDefs["weapon_9mmAR"]=newEntClassDef(Weapon, { CppClass="EntWeapon9mmART"; description="9mm Assault Rifle"; })
EntityClassDefs["weapon_shotgun"]=newEntClassDef(Weapon, { CppClass="EntWeaponShotgunT"; description="Shotgun"; })
EntityClassDefs["weapon_crossbow"]=newEntClassDef(Weapon, { CppClass="EntWeaponCrossbowT"; description="Crossbow"; })
EntityClassDefs["weapon_rpg"]=newEntClassDef(Weapon, { CppClass="EntWeaponRPGT"; description="RPG"; })
EntityClassDefs["weapon_gauss"]=newEntClassDef(Weapon, { CppClass="EntWeaponGaussT"; description="Gauss Gun"; })
EntityClassDefs["weapon_egon"]=newEntClassDef(Weapon, { CppClass="EntWeaponEgonT"; description="Egon Gun"; })
EntityClassDefs["weapon_handgrenade"]=newEntClassDef(Weapon, { CppClass="EntWeaponGrenadeT"; description="Handgrenade Ammo"; })
EntityClassDefs["weapon_tripmine"]=newEntClassDef(Weapon, { CppClass="EntWeaponTripmineT"; description="Tripmine Ammo"; })
EntityClassDefs["weapon_facehugger"]=newEntClassDef(Weapon, { CppClass="EntWeaponFaceHuggerT"; description="Face Hugger (Squeak)"; })

-- Items
EntityClassDefs["item_airtank"]=newEntClassDef(Weapon, { CppClass=""; description="Oxygen tank"; })
EntityClassDefs["item_longjump"]=newEntClassDef(Weapon, { CppClass=""; description="Longjump Module"; })
EntityClassDefs["item_security"]=newEntClassDef(Weapon, { CppClass=""; description="Security card"; })
EntityClassDefs["item_healthkit_small"]=newEntClassDef(Weapon, { CppClass=""; description="Small Health Kit"; })
EntityClassDefs["item_healthkit_big"]=newEntClassDef(Weapon, { CppClass=""; description="Big Health Kit"; })
EntityClassDefs["item_armor_green"]=newEntClassDef(Weapon, { CppClass=""; description="Green Armor"; })
EntityClassDefs["item_armor_yellow"]=newEntClassDef(Weapon, { CppClass=""; description="Yellow Armor"; })
EntityClassDefs["item_armor_red"]=newEntClassDef(Weapon, { CppClass=""; description="Red Armor"; })


-- Static detail models.
-- Note: There is no clipping (collision detection) for these models! If necessary, use explicit "clip" brushes.
EntityClassDefs["static_detail_model"]=newEntClassDef(Common, Angles,
{
    isPoint    =true;
    CppClass   ="EntStaticDetailModelT";
    description="A static detail model.";
    size       ={ { -16, -16, -24 }, { 16, 16, 24 } };
    color      ={ 255, 255, 0 };
    helpers    ={ model={} };

    model=
    {
        type       ="file_model";
        description="Model file name";
    };

    collisionModel=
    {
        type       ="file";
        description="Collision Model file name";
    };

    sequence=
    {
        type       ="integer";
        description="Model sequence";
        value      =0;
    };

    scale=
    {
        type       ="string";
        description="Model scale";
        value      ="1.0";
    };

    gui=
    {
        type       ="file";
        description="GUI file name";
    };
})


EntityClassDefs["speaker"]=newEntClassDef(Common, Angles,
{
    isPoint    =true;
    CppClass   ="EntSpeakerT";
    description="An invisible speaker that can playback a sound shader or file.";

    soundshader=
    {
        type       ="string";
        description="Name of sound shader or file for playback, e.g. Games/DeathMatch/Sounds/jungle.wav";
        value      ="";
    };

    autoplay=
    {
        type       ="flags";    -- This should actually be "bool".
        description="Whether the sound starts playing automatically when the entity is created, or if it has to be triggered first.";
        flags      ={ "autoplay", true };
    };

    interval=
    {
        type       ="float";
        description="Time in seconds between two successive sound playbacks. 0 means that the sound is only played once.";
        value      =5;
    };

    innerCone=
    {
        type       ="integer";
        description="Apex angle of the speakers inner sound cone (where the sound is at its normal volume).";
        value      =360;
    };

    innerVolume=
    {
        type       ="float";
        description="This is the main volume control for the sound (inside the inner cone and before distance attenuation). Typically set between 0.0 and 1.0, but values larger than 1.0 are possible.";
        value      =1;
    };

    outerCone=
    {
        type       ="integer";
        description="Apex angle of the speakers outer sound cone (where the sound is at its outer volume).";
        value      =360;
    };

    outerVolume=
    {
        type       ="float";
        description="Sound volume beyond the outer cone. Relative to the innerVolume, thus the value must be between 0.0 and 1.0.";
        value      =0;
    };
})


-- This entity class represents rigid bodys such as crates, barrels, or arbitrarily complex objects.
-- Their motion is controlled by rigid body dynamics as implemented by the physics engine.
EntityClassDefs["Rigid Body"]=newEntClassDef(Common,
{
    isSolid    =true;
    CppClass   ="EntRigidBodyT";
    description="A rigid body whose motion is controlled by rigid body dynamics as implemented by the physics engine.";

    Mass=
    {
        type       ="float";
        description="The mass of the body, in kilograms [kg]. (1 pound are 0.454 kilograms.)";
        value      =10;
    };

    Shape=
    {
        type       ="choice";
        description="The shape of the body used for the physics computations. \"Visual Elements\" means that the physical shape matches the visual shape.";
        choices    ={ "Box", "Sphere", "Visual Elements" };    -- TODO: Need more default shapes!
    };

    -- TODO: friction, restitution (are these material properties??)
    -- TODO: does explode? can destroy?
    -- TODO: start active??
})
