

class BaseEntityT
{
    public:

    GetName();
    GetOrigin();
    SetOrigin();
};


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


/// All-round trigger entity for activating (calling) script methods when something enters the trigger volume.
///
/// @mapName{Trigger}
/// @cppName{EntTriggerT}
class EntTriggerT : public BaseEntityT
{
    public:

    Activate();
    Deactivate();
    IsActive();
};


/// A static detail model.
///
/// @mapName{static_detail_model}
/// @cppName{EntStaticDetailModelT}
class EntStaticDetailModelT : public BaseEntityT
{
    public:

    IsPlayingAnim();
    PlayAnim();
    GetSequNr();
    SetSequNr();
    RestartSequ();
    GetNumSequences();
};


/// An invisible speaker that can playback a sound shader or file.
///
/// @mapName{speaker}
/// @cppName{EntSpeakerT}
class EntSpeakerT : public BaseEntityT
{
    public:

    Play();
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

    ApplyImpulse();
    SetGravity();
};


/// Dynamic Light Source
///
/// @mapName{PointLightSource}
/// @cppName{EntPointLightSourceT}
class EntPointLightSourceT : public BaseEntityT
{
    public:

    GetColor();
    SetColor();
    GetRadius();
    SetRadius();
};


/// This entity creates other entities ("life forms").
///
/// @mapName{LifeFormMaker}
/// @cppName{EntMonsterMakerT}
class EntMonsterMakerT : public BaseEntityT
{
    public:

};


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
/// It's main purpose is to mark points in the map that the map scripts can refer to.
///
/// @mapName{info_generic}
/// @cppName{EntInfoGenericT}
class EntInfoGenericT : public BaseEntityT
{
    public:

};


/// A human player entity. Should never be placed in a map or even shown in the editor, it just doesn't make sense.
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

    SetOrigin();
    Translate();
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


/// An entity that implements usual, automatic doors.
///
/// @mapName{func_door}
/// @cppName{EntFuncDoorT}
class EntFuncDoorT : public BaseEntityT
{
    public:

    SetOrigin();
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
