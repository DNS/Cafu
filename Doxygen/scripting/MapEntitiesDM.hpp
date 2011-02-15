

/// This is the base class that all other entities directly or indirectly inherit from.
/// The functionality that this class provides is shared by all entities.
/// Entities of this class cannot be created in map files directly, but they're always created via one of the derived classes.
///
/// @cppName{BaseEntityT}
class BaseEntityT
{
    public:

    /// Returns the name of this entity.
    ///
    /// The name of the entity is normally assigned in the map file / map editor,
    /// and unique among all entities.
    /// When a new map starts, a script object that represents the entity is created as a global with this name.
    ///
    /// For example, this function prints the names of all entities in a map:
    /// @code
    ///     function listAllEnts()
    ///         for k, v in pairs(_G) do                      -- Iterate over all global variables.
    ///             if type(v)=="table" and v.GetName then    -- Implement a very simple type check.
    ///                 Console.Print(v:GetName() .. "\n")    -- Note that  v:GetName() == k  is true.
    ///             end
    ///         end
    ///     end
    /// @endcode
    ///
    /// @returns The name that the entity has been assigned in the map file / map editor.
    string GetName();

    /// Returns the origin of this entity as a triple of numbers.
    ///
    /// Example:
    /// @code
    ///     local px, py, pz=Player1:GetOrigin()
    /// @endcode
    ///
    /// @returns The origin of this entity as a triple of numbers.
    x y z GetOrigin();

    /// Sets a new position for this entity.
    ///
    /// @param x   The new x-coordinate for the origin of this entity.
    /// @param y   The new y-coordinate for the origin of this entity.
    /// @param z   The new z-coordinate for the origin of this entity.
    SetOrigin(number x, number y, number z);
};


/// @cppName{EntWeaponT}
class EntWeaponT : public BaseEntityT
{
    public:

};


/// Tripmine Ammo
///
/// @mapName{weapon_tripmine}
/// @cppName{EntWeaponTripmineT}
class EntWeaponTripmineT : public EntWeaponT
{
    public:

};


/// Shotgun
///
/// @mapName{weapon_shotgun}
/// @cppName{EntWeaponShotgunT}
class EntWeaponShotgunT : public EntWeaponT
{
    public:

};


/// RPG
///
/// @mapName{weapon_rpg}
/// @cppName{EntWeaponRPGT}
class EntWeaponRPGT : public EntWeaponT
{
    public:

};


/// 9mm Handgun
///
/// @mapName{weapon_9mmhandgun}
/// @cppName{EntWeaponPistolT}
class EntWeaponPistolT : public EntWeaponT
{
    public:

};


/// Hornet Gun
///
/// @mapName{weapon_hornetgun}
/// @cppName{EntWeaponHornetGunT}
class EntWeaponHornetGunT : public EntWeaponT
{
    public:

};


/// Handgrenade Ammo
///
/// @mapName{weapon_handgrenade}
/// @cppName{EntWeaponGrenadeT}
class EntWeaponGrenadeT : public EntWeaponT
{
    public:

};


/// Gauss Gun
///
/// @mapName{weapon_gauss}
/// @cppName{EntWeaponGaussT}
class EntWeaponGaussT : public EntWeaponT
{
    public:

};


/// Face Hugger (Squeak)
///
/// @mapName{weapon_facehugger}
/// @cppName{EntWeaponFaceHuggerT}
class EntWeaponFaceHuggerT : public EntWeaponT
{
    public:

};


/// Egon Gun
///
/// @mapName{weapon_egon}
/// @cppName{EntWeaponEgonT}
class EntWeaponEgonT : public EntWeaponT
{
    public:

};


/// Crossbow
///
/// @mapName{weapon_crossbow}
/// @cppName{EntWeaponCrossbowT}
class EntWeaponCrossbowT : public EntWeaponT
{
    public:

};


/// Battle Scythe
///
/// @mapName{weapon_battlescythe}
/// @cppName{EntWeaponBattleScytheT}
class EntWeaponBattleScytheT : public EntWeaponT
{
    public:

};


/// 9mm Assault Rifle
///
/// @mapName{weapon_9mmAR}
/// @cppName{EntWeapon9mmART}
class EntWeapon9mmART : public EntWeaponT
{
    public:

};


/// 357 Handgun
///
/// @mapName{weapon_357handgun}
/// @cppName{EntWeapon357T}
class EntWeapon357T : public EntWeaponT
{
    public:

};


/// An all-round trigger entity for calling script methods when something enters the trigger volume.
///
/// A trigger entity is composed of brushes that define the trigger volume.
/// If a player (or in fact, any other entity) walks into the trigger volume,
/// the user-defined OnTrigger(entity Other) method is automatically called,
/// whose given parameter @c Other is the entity that entered the volume and
/// thereby caused the trigger to fire.
///
/// Note that the OnTrigger() callback is only called when the trigger is \emph{active}.
/// Newly created triggers are active by default.
///
/// @mapName{Trigger}
/// @cppName{EntTriggerT}
class EntTriggerT : public BaseEntityT
{
    public:

    /// Activates this trigger.
    Activate();

    /// Deactivates this trigger.
    Deactivate();

    /// Returns whether this trigger entity is currently active.
    boolean IsActive();

    /// User-defined callback for trigger events.
    ///
    /// You do normally \emph{not} call this method yourself, but instead you define it in your map script:
    /// It is called automatically whenever another entity enters the trigger volume.
    ///
    /// See these map script for usage examples:
    ///   - http://trac.cafu.de/browser/cafu/trunk/Games/DeathMatch/Worlds/TechDemo.lua
    ///   - http://trac.cafu.de/browser/cafu/trunk/Games/DeathMatch/Worlds/TestPatches.lua
    ///
    /// @param Other   The other game entity that entered our trigger volume.
    OnTrigger(entity Other);
};


/// A static detail model.
/// A static detail model adds geometric detail to a map and can optionally hold a scripted GUI that the user can interact with.
/// Despite its name, a static detail model can run animated sequences, but note that these animations are essentially a client-side
/// effect only, as only a <em>restart</em> of a sequence is sync'ed over the network.
///
/// @mapName{static_detail_model}
/// @cppName{EntStaticDetailModelT}
class EntStaticDetailModelT : public BaseEntityT
{
    public:

    /// Returns whether the animation sequence is currently playing.
    ///
    /// When the animation sequence is playing, the frames of the sequences are advanced over time normally.
    /// Otherwise, the animation is halted and kept still.
    ///
    /// @returns @c true if the animation sequence is playing, @c false if it is halted.
    boolean IsPlayingAnim();

    /// Plays or halts the animation sequence.
    ///
    /// @param play   If @c true, the animation sequence is played normally.
    ///               If @c false, the frames are no longer advanced over time and thus the sequence is kept still.
    PlayAnim(boolean play);

    /// Returns the number of the current animation sequence.
    ///
    /// @returns The number (an integer) of the current animation sequence.
    number GetSequNr();

    /// Sets the sequence number.
    /// It also resets the frame counter back to 0, that is, the new sequence is played from the beginning.
    ///
    /// @param SequNr   The number of the new animation sequence to use.
    SetSequNr(number SequNr);

    /// Resets the frame counter of the currently playing (or halted) sequence to 0.
    RestartSequ();

    /// Returns the number (an integer) of animation sequences that are available with this static detail model.
    /// Example:
    /// @code
    ///     -- Set the next sequence number, wrap to 0 after the last.
    ///     SDM:SetSequNr((SDM:GetSequNr()+1) % SDM:GetNumSequences())
    /// @endcode
    /// See http://trac.cafu.de/browser/cafu/trunk/Games/DeathMatch/Worlds/Kidney.lua for another example
    /// where the above statement cycles to the next animation sequence in the model every three seconds.
    number GetNumSequences();
};


/// An invisible speaker that can playback a sound shader or file.
/// TODO: Expose more scripting control.
///
/// @mapName{speaker}
/// @cppName{EntSpeakerT}
class EntSpeakerT : public BaseEntityT
{
    public:

    /// Plays the sound associated with this speaker.
    Play();

    /// Stops playing the sound.
    Stop();
};


/// Rocket
///
/// @mapName{monster_rocket}
/// @cppName{EntRocketT}
class EntRocketT : public BaseEntityT
{
    public:

};


/// A rigid body whose motion is controlled by rigid body dynamics as implemented by the physics engine.
///
/// @mapName{Rigid Body}
/// @cppName{EntRigidBodyT}
class EntRigidBodyT : public BaseEntityT
{
    public:

    /// Applies an impulse at the center of this body.
    ApplyImpulse(number ix, number iy, number iz);

    /// Sets the gravity vector for this body.
    /// The default gravity vector is (0, 0, -9.81).
    ///
    /// Example:
    /// @code
    ///     crate_001:SetGravity(0, 0, 9.81)    -- This will "drop" the crate to the *ceiling*!
    /// @endcode
    SetGravity(number gx, number gy, number gz);
};


/// This entity represents a dynamic light source.
/// Dynamic light sources cast dynamic, per-pixel computed light.
/// Its shading details and shadows are updated with each rendered frame.
///
/// EntPointLightSourceT entities are easy to use, but note that too many of them with a too large radius
/// in a single scene can negatively affect the rendering performance.
///
/// See http://www.cafu.de/wiki/mapping:cawe:lighting for more details about the available kinds of light sources and their characteristics.
///
/// @mapName{PointLightSource}
/// @cppName{EntPointLightSourceT}
class EntPointLightSourceT : public BaseEntityT
{
    public:

    /// Returns both the diffuse and the specular color of this light source as a 6-tuple.
    /// @code
    ///     local dr, dg, db, sr, sg, sb = DynLight:GetColor()
    /// @endcode
    /// @returns The diffuse and the specular color of this light source as a 6-tuple.
    dr dg db sr sg sb GetColor();

    /// Sets the color (both diffuse and specular) of this light source.
    ///
    /// The parameters @c r, @c g and @c b are numbers between 0 (no contribution) and 255 (max. contribution) for the respective color component.
    ///
    /// @param r   The red component of the light color.
    /// @param g   The green component of the light color.
    /// @param b   The blue component of the light color.
    SetColor(number r, number g, number b);

    /// Sets the color (diffuse and specular separately) of this light source.
    ///
    /// The parameters are numbers between 0 (no contribution) and 255 (max. contribution) for the respective color component.
    ///
    /// @param dr   The red component of the diffuse light color.
    /// @param dg   The green component of the diffuse light color.
    /// @param db   The blue component of the diffuse light color.
    /// @param sr   The red component of the specular light color.
    /// @param sg   The green component of the specular light color.
    /// @param sb   The blue component of the specular light color.
    SetColor(number dr, number dg, number db, number sr, number sg, number sb);

    /// Returns the radius of this light source.
    ///
    /// The radius determines the distance from the light source origin at which the light intensity has dropped to zero.
    ///
    /// @code
    ///     local r = DynLight:GetRadius()
    /// @endcode
    number GetRadius();

    /// Sets the radius of this light source.
    SetRadius(number r);
};


/// This entity creates other entities ("life forms").
///
/// @mapName{LifeFormMaker}
/// @cppName{EntMonsterMakerT}
class EntMonsterMakerT : public BaseEntityT
{
    public:

};


/// @cppName{EntItemT}
class EntItemT : public BaseEntityT
{
    public:

};


/// Crossbow Arrows
///
/// @mapName{ammo_crossbow_arrows}
/// @cppName{EntItemAmmoArrowT}
class EntItemAmmoArrowT : public EntItemT
{
    public:

};


/// 357 Pistol Ammo
///
/// @mapName{ammo_357}
/// @cppName{EntItemAmmo357T}
class EntItemAmmo357T : public EntItemT
{
    public:

};


/// A starting location for the player.
///
/// @mapName{info_player_start}
/// @cppName{EntInfoPlayerStartT}
class EntInfoPlayerStartT : public BaseEntityT
{
    public:

};


/// A generic entity that provides additional information at any given point in the map.
/// It's main purpose is to mark points in the map that the map scripts can refer to,
/// using the methods of the base class BaseEntityT.
///
/// @mapName{info_generic}
/// @cppName{EntInfoGenericT}
class EntInfoGenericT : public BaseEntityT
{
    public:

};


/// A human player entity.
/// Should never be placed in a map or even shown in the editor, it just doesn't make sense.
///
/// @mapName{HumanPlayer}
/// @cppName{EntHumanPlayerT}
class EntHumanPlayerT : public BaseEntityT
{
    public:

};


/// Handgrenade
///
/// @mapName{monster_handgrenade}
/// @cppName{EntHandGrenadeT}
class EntHandGrenadeT : public BaseEntityT
{
    public:

};


/// An entity for moving objects like doors, lifts, platforms, etc. To be used with scripting.
///
/// @mapName{func_mover}
/// @cppName{EntFuncMoverT}
class EntFuncMoverT : public BaseEntityT
{
    public:

    // No need to document SetOrigin() here again, there is no change in externally visible behaviour.
    // SetOrigin();

    /// Linearly translates this entity to the given position over the given time.
    /// While a translation is in progress, no other translation can be filed (additional calls to Translate() are ignored).
    ///
    /// @param x   The x-coordinate of the position to translate the entity to.
    /// @param y   The y-coordinate of the position to translate the entity to.
    /// @param z   The z-coordinate of the position to translate the entity to.
    /// @param time   The time to take for the translation.
    Translate(number x, number y, number z, number time);

    /// This method is not yet implemented.
    Rotate();
};


/// Ladder (for climbing)
///
/// @mapName{func_ladder}
/// @cppName{EntFuncLadderT}
class EntFuncLadderT : public BaseEntityT
{
    public:

};


/// This entity implements fully automatic doors.
///
/// @mapName{func_door}
/// @cppName{EntFuncDoorT}
class EntFuncDoorT : public BaseEntityT
{
    public:

    /// This method does nothing &ndash; doors are fully automatic now and cannot be moved per script.
    /// Use an EntFuncMoverT entity instead if you want to exercise manual control over moving entities.
    SetOrigin(number x, number y, number z);
};


/// Facehugger
///
/// @mapName{monster_facehugger}
/// @cppName{EntFaceHuggerT}
class EntFaceHuggerT : public BaseEntityT
{
    public:

};


/// Eagle
///
/// @mapName{monster_eagle}
/// @cppName{EntEagleT}
class EntEagleT : public BaseEntityT
{
    public:

};


/// A corpse entity. Usually created by code.
///
/// @mapName{corpse}
/// @cppName{EntCorpseT}
class EntCorpseT : public BaseEntityT
{
    public:

};


/// Company Bot (Trinity)
///
/// @mapName{monster_companybot}
/// @cppName{EntCompanyBotT}
class EntCompanyBotT : public BaseEntityT
{
    public:

};


/// Butterfly
///
/// @mapName{monster_butterfly}
/// @cppName{EntButterflyT}
class EntButterflyT : public BaseEntityT
{
    public:

};


/// AR Grenade
///
/// @mapName{monster_argrenade}
/// @cppName{EntARGrenadeT}
class EntARGrenadeT : public BaseEntityT
{
    public:

};
