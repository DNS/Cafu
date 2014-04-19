/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

#include "CompHumanPlayer.hpp"
#include "AllComponents.hpp"
#include "CompPhysics.hpp"
#include "CompPlayerPhysics.hpp"
#include "CompScript.hpp"
#include "Entity.hpp"
#include "World.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "String.hpp"
#include "UniScriptState.hpp"

#define GAME_NAME
#include "../Games/DeathMatch/Code/Constants_AmmoSlots.hpp"
#include "../Games/DeathMatch/Code/Constants_WeaponSlots.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


// Constants for m_StateOfExistence.
static const uint8_t StateOfExistence_Alive           = 0;
static const uint8_t StateOfExistence_Dead            = 1;
static const uint8_t StateOfExistence_FrozenSpectator = 2;
static const uint8_t StateOfExistence_FreeSpectator   = 3;


const char* ComponentHumanPlayerT::DocClass =
    "Entities with this component are associated with a client connection\n"
    "at whose end is a human player who provides input to control the entity.";


const cf::TypeSys::VarsDocT ComponentHumanPlayerT::DocVars[] =
{
    { "PlayerName",          "The name that the player chose for himself." },
    { "State",               "For the player's main state machine, e.g. spectator, dead, alive, ..." },
    { "Health",              "Health." },
    { "Armor",               "Armor." },
    { "HaveItems",           "Bit field, entity can carry 32 different items." },
    { "HaveWeapons",         "Bit field, entity can carry 32 different weapons." },
    { "ActiveWeaponSlot",    "Index into m_HaveWeapons, m_HaveAmmoInWeapons, and for determining the weapon model index." },
    { "ActiveWeaponSequNr",  "The weapon anim sequence that we see (the local clients 1st person ('view') weapon model)." },
    { "ActiveWeaponFrameNr", "Respectively, this is the frame number of the current weapon sequence." },
    { "HaveAmmo",            "Entity can carry 16 different types of ammo (weapon independent). This is the amount of each." },
    { "HaveAmmoInWeapons",   "Entity can carry ammo in each of the 32 weapons. This is the amount of each." },
    { NULL, NULL }
};


ComponentHumanPlayerT::ComponentHumanPlayerT()
    : ComponentBaseT(),
      m_PlayerName("PlayerName", "Player"),
      m_StateOfExistence("State", 2 /*StateOfExistence_FrozenSpectator*/),
      m_Health("Health", 100),
      m_Armor("Armor", 0),
      m_HaveItems("HaveItems", 0),
      m_HaveWeapons("HaveWeapons", 0),
      m_ActiveWeaponSlot("ActiveWeaponSlot", 0),
      m_ActiveWeaponSequNr("ActiveWeaponSequNr", 0),
      m_ActiveWeaponFrameNr("ActiveWeaponFrameNr", 0.0f),
      m_HaveAmmo("HaveAmmo", 16, 0),
      m_HaveAmmoInWeapons("HaveAmmoInWeapons", 32, 0),
      m_PlayerCommands(),
      m_GuiHUD(NULL)
{
    FillMemberVars();
}


ComponentHumanPlayerT::ComponentHumanPlayerT(const ComponentHumanPlayerT& Comp)
    : ComponentBaseT(Comp),
      m_PlayerName(Comp.m_PlayerName),
      m_StateOfExistence(Comp.m_StateOfExistence),
      m_Health(Comp.m_Health),
      m_Armor(Comp.m_Armor),
      m_HaveItems(Comp.m_HaveItems),
      m_HaveWeapons(Comp.m_HaveWeapons),
      m_ActiveWeaponSlot(Comp.m_ActiveWeaponSlot),
      m_ActiveWeaponSequNr(Comp.m_ActiveWeaponSequNr),
      m_ActiveWeaponFrameNr(Comp.m_ActiveWeaponFrameNr),
      m_HaveAmmo(Comp.m_HaveAmmo),
      m_HaveAmmoInWeapons(Comp.m_HaveAmmoInWeapons),
      m_PlayerCommands(),
      m_GuiHUD(NULL)
{
    FillMemberVars();
}


void ComponentHumanPlayerT::FillMemberVars()
{
    GetMemberVars().Add(&m_PlayerName);
    GetMemberVars().Add(&m_StateOfExistence);
    GetMemberVars().Add(&m_Health);
    GetMemberVars().Add(&m_Armor);
    GetMemberVars().Add(&m_HaveItems);
    GetMemberVars().Add(&m_HaveWeapons);
    GetMemberVars().Add(&m_ActiveWeaponSlot);
    GetMemberVars().Add(&m_ActiveWeaponSequNr);
    GetMemberVars().Add(&m_ActiveWeaponFrameNr);
    GetMemberVars().Add(&m_HaveAmmo);
    GetMemberVars().Add(&m_HaveAmmoInWeapons);
}


// The HUD GUI is intentionally not initialized in the constructors above, but only lazily
// when the GetGui() method is called, because in the constructors it is impossile to know
// or to learn if this ComponentHumanPlayerT instance is created on the server or the client,
// and if it is the local, first-person human player, or somebody else.
IntrusivePtrT<cf::GuiSys::GuiImplT> ComponentHumanPlayerT::GetGuiHUD()
{
    if (m_GuiHUD != NULL) return m_GuiHUD;

    // TODO:
    // if (on server) return NULL;
    // if (this human player is not first person instance) return NULL;

    static const char* FallbackGUI =
        "local gui = ...\n"
        "local Root = gui:new('WindowT', 'Root')\n"
        "gui:SetRootWindow(Root)\n"
        "\n"
        "function Root:OnInit()\n"
        "    self:GetTransform():set('Pos', 0, 0)\n"
        "    self:GetTransform():set('Size', 640, 480)\n"
        "\n"
        "    local c1 = gui:new('ComponentTextT')\n"
        "    c1:set('Text', [=====[%s]=====])\n"    // This is intended for use with e.g. wxString::Format().
        " -- c1:set('Font', 'Fonts/Impact')\n"
        "    c1:set('Scale', 0.3)\n"
        "    c1:set('Padding', 0, 0)\n"
        "    c1:set('Color', 15/255, 49/255, 106/255)\n"
        " -- c1:set('Alpha', 0.5)\n"
        "    c1:set('hor. Align', 0)\n"
        "    c1:set('ver. Align', 0)\n"
        "\n"
        "    local c2 = gui:new('ComponentImageT')\n"
        "    c2:set('Material', '')\n"
        "    c2:set('Color', 150/255, 170/255, 204/255)\n"
        "    c2:set('Alpha', 0.7)\n"
        "\n"
        "    self:AddComponent(c1, c2)\n"
        "\n"
        "    gui:activate      (true)\n"
        "    gui:setInteractive(false)\n"
        "    gui:showMouse     (false)\n"
        "    gui:setFocus      (Root)\n"
        "end\n";

    WorldT& World = GetEntity()->GetWorld();
    static const std::string GuiName = "Games/DeathMatch/GUIs/HUD_main.cgui";

    try
    {
        m_GuiHUD = new cf::GuiSys::GuiImplT(
            World.GetScriptState(),
            World.GetGuiResources());

        // Set the GUI object's "Player" field to the related component instance (`this`),
        // and the GUI object's "Entity" field to the related entity instance.
        // Expressed as pseudo code:
        //     gui.Player = this
        //     gui.Entity = this->GetEntity()
        {
            lua_State*    LuaState = World.GetScriptState().GetLuaState();
            StackCheckerT StackChecker(LuaState);
            ScriptBinderT Binder(LuaState);

            Binder.Push(m_GuiHUD);
            Binder.Push(IntrusivePtrT<ComponentHumanPlayerT>(this));
            lua_setfield(LuaState, -2, "Player");
            Binder.Push(IntrusivePtrT<EntityT>(GetEntity()));
            lua_setfield(LuaState, -2, "Entity");
            lua_pop(LuaState, 1);
        }

        m_GuiHUD->LoadScript(GuiName);

        // Active status is not really relevant for our Gui that is not managed by the GuiMan,
        // but still make sure that clock tick events are properly propagated to all windows.
        m_GuiHUD->Activate();
    }
    catch (const cf::GuiSys::GuiImplT::InitErrorT& IE)
    {
        // Need a new GuiImplT instance here, as the one allocated above is in unknown state.
        m_GuiHUD = new cf::GuiSys::GuiImplT(
            World.GetScriptState(),
            World.GetGuiResources());

        // This one must not throw again...
        m_GuiHUD->LoadScript(
            cf::String::Replace(FallbackGUI, "%s", "Could not load GUI\n" + GuiName + "\n\n" + IE.what()),
            cf::GuiSys::GuiImplT::InitFlag_InlineCode);
    }

    return m_GuiHUD;
}


Vector3dT ComponentHumanPlayerT::GetPlayerVelocity() const
{
    if (!GetEntity())
        return Vector3dT();

    IntrusivePtrT<ComponentPlayerPhysicsT> CompPlayerPhysics =
        dynamic_pointer_cast<ComponentPlayerPhysicsT>(GetEntity()->GetComponent("PlayerPhysics"));

    if (CompPlayerPhysics == NULL)
        return Vector3dT();

    return CompPlayerPhysics->GetVelocity();
}


Vector3dT ComponentHumanPlayerT::GetOriginWS() const
{
    if (!GetEntity()) return Vector3dT();

    return GetEntity()->GetTransform()->GetOriginWS().AsVectorOfDouble();
}


Vector3dT ComponentHumanPlayerT::GetViewDirWS(double Random) const
{
    if (!GetEntity()) return Vector3dT();

    IntrusivePtrT<ComponentTransformT> Trafo = GetEntity()->GetTransform();

    if (GetEntity()->GetChildren().Size() > 0)
    {
        // The normal, expected case: Use the entity's camera transform.
        Trafo = GetEntity()->GetChildren()[0]->GetTransform();
    }

    const cf::math::Matrix3x3fT Mat(Trafo->GetQuatWS());

    Vector3dT ViewDir = Mat.GetAxis(0).AsVectorOfDouble();

    if (Random > 0.0)
    {
        ViewDir += Mat.GetAxis(0).AsVectorOfDouble() * ((rand() % 10000 - 5000) / 5000.0) * Random;
        ViewDir += Mat.GetAxis(2).AsVectorOfDouble() * ((rand() % 10000 - 5000) / 5000.0) * Random;

        ViewDir = normalizeOr0(ViewDir);
    }

    return ViewDir;
}


RayResultT ComponentHumanPlayerT::TracePlayerRay(const Vector3dT& Dir) const
{
    if (!GetEntity())
        return RayResultT(NULL);

    IntrusivePtrT<ComponentPhysicsT> Physics =
        dynamic_pointer_cast<ComponentPhysicsT>(GetEntity()->GetComponent("Physics"));

    RayResultT RayResult(Physics != NULL ? Physics->GetRigidBody() : NULL);

    GetEntity()->GetWorld().GetPhysicsWorld()->TraceRay(
        UnitsToPhys(GetEntity()->GetTransform()->GetOriginWS().AsVectorOfDouble()),
        Dir * 9999.0, RayResult);

    return RayResult;
}


void ComponentHumanPlayerT::InflictDamage(EntityT* OtherEnt, float Amount, const Vector3dT& Dir) const
{
    if (!OtherEnt) return;

    IntrusivePtrT<ComponentScriptT> OtherScript =
        dynamic_pointer_cast<ComponentScriptT>(OtherEnt->GetComponent("Script"));

    if (OtherScript == NULL) return;

    cf::ScriptBinderT      OtherBinder(OtherEnt->GetWorld().GetScriptState().GetLuaState());
    IntrusivePtrT<EntityT> This = GetEntity();

    OtherBinder.Push(This);

    OtherScript->CallLuaMethod("TakeDamage", 1, "ffff", Amount, Dir.x, Dir.y, Dir.z);
}


ComponentHumanPlayerT* ComponentHumanPlayerT::Clone() const
{
    return new ComponentHumanPlayerT(*this);
}


void ComponentHumanPlayerT::DoServerFrame(float t)
{
    // It is important that we advance the time on the server-side GUI, too, so that it can
    // for example work off the "pending interpolations" that the GUI scripts can create.
    //
    // TODO: Check if this is true, especially in the light of client prediction.
    //       Maybe we should move all HUD GUI code into its own component, thereby
    //       isolating it from all other Human Player concerns, especially prediction?!
    //
    if (GetGuiHUD() != NULL)
        GetGuiHUD()->DistributeClockTickEvents(t);
}


void ComponentHumanPlayerT::DoClientFrame(float t)
{
    if (GetGuiHUD() != NULL)
        GetGuiHUD()->DistributeClockTickEvents(t);

    // TODO: Rendering the HUD GUI should probably be moved into some PostRender() method...
    if (GetGuiHUD() == NULL) return;

    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW);

    const float zNear = 0.0f;
    const float zFar  = 1.0f;
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,     MatrixT::GetProjOrthoMatrix(0.0f, cf::GuiSys::VIRTUAL_SCREEN_SIZE_X, cf::GuiSys::VIRTUAL_SCREEN_SIZE_Y, 0.0f, zNear, zFar));
    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW,  MatrixT());

    GetGuiHUD()->Render();

    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW);
}


static const cf::TypeSys::MethsDocT META_GetCrosshairInfo =
{
    "GetCrosshairInfo",
    "This method is called by the HUD GUI in order to learn which cross-hair should currently be shown.",
    "string", "()"
};

int ComponentHumanPlayerT::GetCrosshairInfo(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentHumanPlayerT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentHumanPlayerT> >(1);

    if (Comp->m_StateOfExistence.Get() != StateOfExistence_Alive)
        return 0;

    switch (Comp->m_ActiveWeaponSlot.Get())
    {
        case WEAPON_SLOT_HORNETGUN:
        case WEAPON_SLOT_PISTOL:
        case WEAPON_SLOT_CROSSBOW:
        case WEAPON_SLOT_357:
        case WEAPON_SLOT_9MMAR:
            lua_pushstring(LuaState, "Gui/CrossHair1");
            return 1;

        case WEAPON_SLOT_SHOTGUN:
        case WEAPON_SLOT_RPG:
        case WEAPON_SLOT_GAUSS:
        case WEAPON_SLOT_EGON:
            lua_pushstring(LuaState, "Gui/CrossHair2");
            lua_pushboolean(LuaState, 1);   // Push "true" to have the GUI apply a continuous rotation to the crosshair image.
            return 2;

        default:
            // Some weapons just don't have a crosshair.
            break;
    }

    return 0;
}


static const cf::TypeSys::MethsDocT META_GetAmmoString =
{
    "GetAmmoString",
    "This method is called by the HUD GUI in order to learn which info should currently be shown in the \"ammo\" field.",
    "string", "()"
};

int ComponentHumanPlayerT::GetAmmoString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentHumanPlayerT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentHumanPlayerT> >(1);

    // Return an ammo string for the players HUD.
    if (Comp->m_HaveWeapons.Get() & (1 << Comp->m_ActiveWeaponSlot.Get()))
    {
        char PrintBuffer[64];

        // Assignment table to determine which ammo is consumed by each weapon for primary fire (given a weapon slot, determine the ammo slot).
        // TODO: This is not optimal, ought to be static member function of each weapon???
        const char GetAmmoSlotForPrimaryFireByWeaponSlot[13] =
        {
            AMMO_SLOT_NONE,
            AMMO_SLOT_NONE,
            AMMO_SLOT_9MM,
            AMMO_SLOT_357,
            AMMO_SLOT_SHELLS,
            AMMO_SLOT_9MM,
            AMMO_SLOT_ARROWS,
            AMMO_SLOT_ROCKETS,
            AMMO_SLOT_CELLS,
            AMMO_SLOT_CELLS,
            AMMO_SLOT_NONE,
            AMMO_SLOT_NONE,
            AMMO_SLOT_NONE
        };

        switch (Comp->m_ActiveWeaponSlot.Get())
        {
            case WEAPON_SLOT_BATTLESCYTHE:
            case WEAPON_SLOT_HORNETGUN:
                lua_pushstring(LuaState, "");
                break;

            case WEAPON_SLOT_9MMAR:
                sprintf(PrintBuffer, "Ammo %2u (%2u) | %u Grenades",
                        Comp->m_HaveAmmoInWeapons[WEAPON_SLOT_9MMAR],
                        Comp->m_HaveAmmo[GetAmmoSlotForPrimaryFireByWeaponSlot[WEAPON_SLOT_9MMAR]],
                        Comp->m_HaveAmmo[AMMO_SLOT_ARGREN]);
                lua_pushstring(LuaState, PrintBuffer);
                break;

            case WEAPON_SLOT_FACEHUGGER:
            case WEAPON_SLOT_GRENADE:
            case WEAPON_SLOT_RPG:
            case WEAPON_SLOT_TRIPMINE:
                sprintf(PrintBuffer, "Ammo %2u",
                        Comp->m_HaveAmmoInWeapons[Comp->GetActiveWeaponSlot()]);
                lua_pushstring(LuaState, PrintBuffer);
                break;

            case WEAPON_SLOT_357:
            case WEAPON_SLOT_CROSSBOW:
            case WEAPON_SLOT_EGON:
            case WEAPON_SLOT_GAUSS:
            case WEAPON_SLOT_PISTOL:
            case WEAPON_SLOT_SHOTGUN:
                sprintf(PrintBuffer, "Ammo %2u (%2u)",
                        Comp->m_HaveAmmoInWeapons[Comp->GetActiveWeaponSlot()],
                        Comp->m_HaveAmmo[GetAmmoSlotForPrimaryFireByWeaponSlot[Comp->GetActiveWeaponSlot()]]);
                lua_pushstring(LuaState, PrintBuffer);
                break;
        }
    }
    else
    {
        // Let the HUD know that we have no weapon.
        lua_pushstring(LuaState, "");
    }

    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentHumanPlayerT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "human player component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentHumanPlayerT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentHumanPlayerT();
}

const luaL_Reg ComponentHumanPlayerT::MethodsList[] =
{
    { "GetCrosshairInfo", GetCrosshairInfo },
    { "GetAmmoString",    GetAmmoString },
    { "__tostring",       toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentHumanPlayerT::DocMethods[] =
{
    META_GetCrosshairInfo,
    META_GetAmmoString,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentHumanPlayerT::TypeInfo(GetComponentTIM(), "GameSys::ComponentHumanPlayerT", "GameSys::ComponentBaseT", ComponentHumanPlayerT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
