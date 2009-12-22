/****************************************************/
/*** USAF Visuo Spatial Working Memory DLL (Code) ***/
/****************************************************/

#include "../../BaseEntity.hpp"
#include "../../GameWorld.hpp"
#include "Libs/LookupTables.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "ConsoleCommands/Console.hpp"
#include "GuiSys/GuiMan.hpp"
#include "FileSys/FileMan.hpp"
#include "MaterialSystem/MaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "MaterialSystem/TextureMap.hpp"
#include "OpenGL/OpenGLWindow.hpp"

#include "EntityCreateParams.hpp"
#include "TypeSys.hpp"

#include <string.h>

#include "Constants_EntityTypeIDs.hpp"
#include "HumanPlayer.hpp"
#include "InfoPlayerStart.hpp"
#include "InfoNodeSpacing.hpp"
#include "Info2DMoveDir.hpp"

#ifdef _WIN32
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
#define __stdcall
#define DLL_EXPORT extern "C"
#endif


// DLL Entry Point
// ***************

#ifdef _WIN32
int __stdcall DllMain(void* /*hInstance*/, unsigned long Reason, void* /*Reserved*/)
{
    // Remeber:
    // a) When the same DLL is loaded multiple times at once (as in single-player games, where BOTH the server AND the client call LoadLibrary()),
    //    we see 'DLL_PROCESS_ATTACH' only once, when it is first mapped to the main executables address space!
    //    The same holds for 'DLL_PROCESS_DETACH', respectively.
    //    Therefore, this is the place to init/shutdown "global" stuff that is shared (neither client nor server specific).
    // b) Global static data/state is *shared*, like the 'LookupTables', the C runtime, the models of the 'ModelManager' and so on!
    if (Reason==1/*DLL_PROCESS_ATTACH*/)
    {
        LookupTables::Initialize();
    }

    return 1;
}
#else
void __attribute__((constructor)) InitializeDLL()
{
    LookupTables::Initialize();
}
#endif


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& GetBaseEntTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


// Entity State
// ************

EntityStateT::EntityStateT(const VectorT& Origin_, const VectorT& Velocity_, const BoundingBox3T<double>& Dimensions_,
                           unsigned short Heading_, unsigned short Pitch_, unsigned short Bank_,
                           char StateOfExistance_, char Flags_, char ModelIndex_, char ModelSequNr_, float ModelFrameNr_,
                           char Health_, char Armor_, unsigned long HaveItems_, unsigned long HaveWeapons_,
                           char ActiveWeaponSlot_, char ActiveWeaponSequNr_, float ActiveWeaponFrameNr_)
    : Origin(Origin_),
      Velocity(Velocity_),
      Dimensions(Dimensions_),

      Heading(Heading_),
      Pitch(Pitch_),
      Bank(Bank_),

      StateOfExistance(StateOfExistance_),
      Flags(Flags_),
   // PlayerName[]
      ModelIndex(ModelIndex_),
      ModelSequNr(ModelSequNr_),
      ModelFrameNr(ModelFrameNr_),

      Health(Health_),
      Armor(Armor_),
      HaveItems(HaveItems_),
      HaveWeapons(HaveWeapons_),
      ActiveWeaponSlot(ActiveWeaponSlot_),
      ActiveWeaponSequNr(ActiveWeaponSequNr_),
      ActiveWeaponFrameNr(ActiveWeaponFrameNr_),
   // HaveAmmo[]
   // HaveAmmoInWeapons[]
      Events(0)
{
    PlayerName[0]=0;

    char Nr;

    for (Nr=0; Nr<16; Nr++) HaveAmmo         [Nr]=0;
    for (Nr=0; Nr<32; Nr++) HaveAmmoInWeapons[Nr]=0;
}


// Base Entity
// ***********

BaseEntityT::BaseEntityT(const EntityCreateParamsT& Params, const EntityStateT& State_)
    : ID(Params.ID),
      Properties(Params.Properties),
      WorldFileIndex(Params.WorldFileIndex),
      ParentID(0xFFFFFFFF),
      State(State_),
      GameWorld(Params.GameWorld),
      CollisionModel(Params.CollisionModel),
      ClipModel(GameWorld->GetClipWorld())
{
    // Evaluate the common 'PropertyPairs'.
    std::map<std::string, std::string>::const_iterator It=Properties.find("angles");

    if (It!=Properties.end())
    {
        char s[1024];

        strncpy(s, It->second.c_str(), 1023);
        s[1023]=0;

        /* const char* s1=*/ strtok(s, " ");
        const char* s2=strtok(NULL, " "); if (s2) State.Heading=(unsigned short)(atof(s2)*8192.0/45.0);
    }
}


BaseEntityT::~BaseEntityT()
{
}


void BaseEntityT::ProcessConfigString(const void* /*ConfigData*/, const char* /*ConfigString*/)
{
}


void BaseEntityT::NotifyTouchedBy(BaseEntityT* /*Entity*/)
{
}


void BaseEntityT::OnTrigger(BaseEntityT* /*Activator*/)
{
}


void BaseEntityT::OnPush(ArrayT<BaseEntityT*>& /*Pushers*/, const Vector3dT& /*PushVector*/)
{
}


void BaseEntityT::TakeDamage(BaseEntityT* /*Entity*/, char /*Amount*/, const VectorT& /*ImpactDir*/)
{
}


void BaseEntityT::Think(float /*FrameTime*/, unsigned long /*ServerFrameNr*/)
{
}


void BaseEntityT::Cl_UnserializeFrom()
{
}


void BaseEntityT::ProcessEvent(char /*EventID*/) const
{
}


bool BaseEntityT::GetLightSourceInfo(unsigned long& /*DiffuseColor*/, unsigned long& /*SpecularColor*/, VectorT& /*Position*/, float& /*Radius*/, bool& /*CastsShadows*/) const
{
    return false;
}


bool BaseEntityT::DrawInterpolated() const
{
    return true;
}


void BaseEntityT::Draw(bool /*FirstPersonView*/, float /*LodDist*/) const
{
}


void BaseEntityT::PostDraw(float /*FrameTime*/, bool /*FirstPersonView*/)
{
}


const cf::TypeSys::TypeInfoT* BaseEntityT::GetType() const
{
    return &TypeInfo;
 // return &BaseEntityT::TypeInfo;
}

// void* BaseEntityT::CreateInstance(unsigned long ID, unsigned long MapFileID, const EngineFunctionsT* EF, const VectorT& Origin)
// {
//     Console->Warning("Cannot instantiate abstract class!\n");
//     assert(false);
//     return NULL;
// }

const cf::TypeSys::TypeInfoT BaseEntityT::TypeInfo(GetBaseEntTIM(), "BaseEntityT", NULL /*No base class.*/, NULL /*BaseEntityT::CreateInstance*/, NULL /*MethodsList*/);

unsigned long BaseEntityT::GetTypeNr() const
{
    return GetType()->TypeNr;
}


// Exported DLL Functions
// **********************


// This is the first function that is called by both the client and the server after they have loaded this DLL.
// Its purpose is to point us to the shared implementation of the relevant interfaces (the MatSys etc.),
// so that we can access the same implementation of the interfaces as the engine.
//
// The fact that DLLs that are loaded multiple times cause only a reference counter to be increased rather than separate copies
// of the DLL to be created (the global state exists only once), and the way how clients and servers change worlds (client deletes
// the old world first, then loads the new, server loads new world first and only then deletes the old one), and the fact that in
// a single Ca3DE.exe instance, the client only, the server only, or both can be running, means that a *single* instance of this
// DLL may live over several world changes of a client and server, because at least one of them keeps referring to it at all times.
//
// Therefore, it may happen that InitInterfaces() is called *many* times, namely on each world change once by the server and once
// by the client. The parameters to this function however are always non-volatile, they don't change over multiple calls.
// In future implementations I'll possibly change this and load and init the DLL only once, even before the client or server gets instantiated.
SingleOpenGLWindowT*   SingleOpenGLWindow=NULL;
cf::GuiSys::GuiManI*   cf::GuiSys::GuiMan=NULL;     // Define the global GuiMan pointer instance -- see GuiMan.hpp for more details.
cf::ConsoleI*          Console=NULL;
ConsoleInterpreterI*   ConsoleInterpreter=NULL;
cf::FileSys::FileManI* cf::FileSys::FileMan=NULL;

DLL_EXPORT void __stdcall InitInterfaces(SingleOpenGLWindowT* SingleWin, MatSys::RendererI* Renderer, MatSys::TextureMapManagerI* TexMapMan, MaterialManagerI* MatMan, cf::GuiSys::GuiManI* GuiMan_, cf::ConsoleI* Console_, ConsoleInterpreterI* ConInterpreter_)
{
    SingleOpenGLWindow       =SingleWin;
    MatSys::Renderer         =Renderer;
    MatSys::TextureMapManager=TexMapMan;
    MaterialManager          =MatMan;
    cf::GuiSys::GuiMan       =GuiMan_;
    Console                  =Console_;
    ConsoleInterpreter       =ConInterpreter_;
 // cf::FileSys::FileMan     =TODO!!!;

    // This won't be possible here:
    //     EngineInterface=EngInt;
    // because each entity needs an individual pointer (as is now)!
    // This is because entities may "live" in a client world or in a server world, and accordingly need pointers to respective, different implementations.
    // Moreover, during world changes, entities that exist in *different* worlds may shortly exist simultaneously - even more
    // need to provide each with a pointer to its appropriate engine interface.
    // Mhhh. A *better* name would be "World Interface"...
}


// This function is called by the engine, whenever the client/server loads/unloads the game DLL.
// The purpose is to provide opportunities for init and shut-down code that depend on whether the
// game DLL is loaded/unloaded for the client or for the server.
// Practical relevance: If this is called for the client, we may assume that the audio and video
// sub-systems (OpenGL, FMOD, ...) are already initialized or about to be shutdown, respectively.
DLL_EXPORT void __stdcall AdminGameDll(const char /*Reason*/)
{
}


// This function is called by the server, in order to obtain a (pointer to a) 'BaseEntityT' from a map file entity.
// The server also provides the ID and engine function call-backs for the new entity.
DLL_EXPORT BaseEntityT* __stdcall CreateBaseEntityFromMapFile(const char* MapFileName, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin)
{
    if (MapFileName==0) return 0;

    if (strcmp(MapFileName, "HumanPlayer"           )==0) return new EntHumanPlayerT    (TYPEID_HUMANPLAYER,     ID, MapFileID, GameWorld, Origin);
    if (strcmp(MapFileName, "info_player_start"     )==0) return new EntInfoPlayerStartT(TYPEID_INFOPLAYERSTART, ID, MapFileID, GameWorld, Origin);
    if (strcmp(MapFileName, "info_node_spacing"     )==0) return new EntInfoNodeSpacingT(TYPEID_INFONODESPACING, ID, MapFileID, GameWorld, Origin);
    if (strcmp(MapFileName, "info_2d_move_direction")==0) return new EntInfo2DMoveDirT  (TYPEID_INFO2DMOVEDIR,   ID, MapFileID, GameWorld, Origin);

    return 0;
}


// This function is called by the client, in order to obtain a (pointer to a) 'BaseEntityT' for a new entity
// whose type and ID it got via a net message from the server.
// (It initializes the 'State' of the entity directly via the returned pointer.)
// The client also provides engine function call-backs, such that the prediction feature can work.
DLL_EXPORT BaseEntityT* __stdcall CreateBaseEntityFromTypeID(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld)
{
    if (TypeID==TYPEID_HUMANPLAYER    ) return new EntHumanPlayerT    (TypeID, ID, MapFileID, GameWorld, VectorT());
    if (TypeID==TYPEID_INFOPLAYERSTART) return new EntInfoPlayerStartT(TypeID, ID, MapFileID, GameWorld, VectorT());
    if (TypeID==TYPEID_INFONODESPACING) return new EntInfoNodeSpacingT(TypeID, ID, MapFileID, GameWorld, VectorT());
    if (TypeID==TYPEID_INFO2DMOVEDIR  ) return new EntInfo2DMoveDirT  (TypeID, ID, MapFileID, GameWorld, VectorT());

    return 0;
}


// Called by both the client and the server to release previously obtained 'BaseEntityT's.
// Note that simply deleting them directly is not possible (the "EXE vs. DLL boundary").
DLL_EXPORT void __stdcall FreeBaseEntity(BaseEntityT* BaseEntity)
{
    delete BaseEntity;
}
