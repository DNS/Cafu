/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "ServerWorld.hpp"
#include "../Both/EntityManager.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/Console.hpp"      // For cf::va().
#include "SceneGraph/BspTreeNode.hpp"
#include "../../Games/Game.hpp"


static ConVarT AutoAddCompanyBot("sv_AutoAddCompanyBot", false, ConVarT::FLAG_MAIN_EXE, "If true, auto-inserts an entity of type \"Company Bot\" at the player start position into each newly loaded map.");


CaServerWorldT::CaServerWorldT(const char* FileName, Ca3DEWorldT* Ca3DEWorld_)
    : Ca3DEWorld(Ca3DEWorld_),
      EntityManager(*Ca3DEWorld->EntityManager),
      ServerFrameNr(1)  // 0 geht nicht, denn die ClientInfoT::LastKnownFrameReceived werden mit 0 initialisiert!
{
    cf::GameSys::Game->Sv_PrepareNewWorld(FileName, Ca3DEWorld->CollModel);

    // Gehe alle GameEntities der Ca3DEWorld durch und erstelle dafür "echte" Entities.
    for (unsigned long GENr=0; GENr<Ca3DEWorld->GameEntities.Size(); GENr++)
    {
        const GameEntityT* GE=Ca3DEWorld->GameEntities[GENr];

        // Register GE->CollModel also with the cf::ClipSys::CollModelMan, so that both the owner (the game entity GE)
        // as well as the game code can free/delete it in their destructors (one by "delete", the other by cf::ClipSys::CollModelMan->FreeCM()).
        cf::ClipSys::CollModelMan->GetCM(GE->CollModel);

        EntityManager.CreateNewEntityFromBasicInfo(GE->Properties, GE->BspTree, GE->CollModel, GENr, GE->MFIndex, ServerFrameNr, GE->Origin);
    }

    // Gehe alle InfoPlayerStarts der Ca3DEWorld durch und erstelle dafür "echte" Entities.
    for (unsigned long IPSNr=0; IPSNr<Ca3DEWorld->InfoPlayerStarts.Size(); IPSNr++)
    {
        std::map<std::string, std::string> Props;

        Props["classname"]="info_player_start";
        Props["angles"]   =cf::va("0 %lu 0", (unsigned long)(Ca3DEWorld->InfoPlayerStarts[IPSNr].Heading/8192.0*45.0));

        EntityManager.CreateNewEntityFromBasicInfo(Props, NULL, NULL, (unsigned long)-1, (unsigned long)-1, ServerFrameNr, Ca3DEWorld->InfoPlayerStarts[IPSNr].Origin);
    }

    // Zu Demonstrationszwecken fügen wir auch noch einen MonsterMaker vom Typ CompanyBot in die World ein.
    // TODO! Dies sollte ins Map/Entity-Script wandern!!!
    if (AutoAddCompanyBot.GetValueBool())
    {
        std::map<std::string, std::string> Props;

        Props["classname"]         ="LifeFormMaker";
        Props["angles"]            =cf::va("0 %lu 0", (unsigned long)(Ca3DEWorld->InfoPlayerStarts[0].Heading/8192.0*45.0));
        Props["monstertype"]       ="CompanyBot";
        Props["monstercount"]      ="1";
        Props["m_imaxlivechildren"]="1";

        EntityManager.CreateNewEntityFromBasicInfo(Props, NULL, NULL, (unsigned long)-1, (unsigned long)-1, ServerFrameNr, Ca3DEWorld->InfoPlayerStarts[0].Origin);
    }

    cf::GameSys::Game->Sv_FinishNewWorld(FileName);
}


CaServerWorldT::~CaServerWorldT()
{
    delete Ca3DEWorld;                      // Unregisters all entities from the games script state.
    cf::GameSys::Game->Sv_UnloadWorld();    // Deletes the games script state.
}


unsigned long CaServerWorldT::InsertHumanPlayerEntityForNextFrame(const char* PlayerName, const char* ModelName, unsigned long ClientInfoNr)
{
    std::map<std::string, std::string> Props;

    Props["classname"]="HumanPlayer";
    Props["name"]     =cf::va("Player%lu", ClientInfoNr+1);     // Setting the name is needed so that player entities can have a corresponding script instance.
    Props["angles"]   =cf::va("0 %lu 0", (unsigned long)(Ca3DEWorld->InfoPlayerStarts[0].Heading/8192.0*45.0));

    return EntityManager.CreateNewEntityFromBasicInfo(Props, NULL, NULL, (unsigned long)-1, (unsigned long)-1, ServerFrameNr+1, Ca3DEWorld->InfoPlayerStarts[0].Origin+VectorT(0, 0, 1000), PlayerName, ModelName);
}


void CaServerWorldT::RemoveHumanPlayerEntity(unsigned long HumanPlayerEntityID)
{
    EntityManager.RemoveEntity(HumanPlayerEntityID);
}


void CaServerWorldT::NotifyHumanPlayerEntityOfClientCommand(unsigned long HumanPlayerEntityID, const PlayerCommandT& PlayerCommand)
{
    EntityManager.ProcessConfigString(HumanPlayerEntityID, &PlayerCommand, "PlayerCommand");
}


void CaServerWorldT::Think(float FrameTime)
{
    // Zuerst die Nummer des nächsten Frames 'errechnen'.
    // Die Reihenfolge ist wichtig, denn wenn ein neuer Entity geschaffen wird,
    // muß dieser korrekt wissen, zu welchem Frame er ins Leben gerufen wurde.
    ServerFrameNr++;

    // Jetzt das eigentliche Denken durchführen.
    // Herauskommen tut eine Aussage der Form: "Zum Frame Nummer 'ServerFrameNr' ist die World in diesem Zustand!"
    EntityManager.Think(FrameTime, ServerFrameNr);
}


unsigned long CaServerWorldT::WriteClientNewBaseLines(unsigned long OldBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const
{
    EntityManager.WriteNewBaseLines(OldBaseLineFrameNr, OutDatas);

    return ServerFrameNr;
}


void CaServerWorldT::WriteClientDeltaUpdateMessages(unsigned long ClientEntityID, unsigned long ClientFrameNr, ArrayT< ArrayT<unsigned long> >& ClientOldStatesPVSEntityIDs, unsigned long& ClientCurrentStateIndex, NetDataT& OutData) const
{
    EntityManager.WriteFrameUpdateMessages(ClientEntityID, ServerFrameNr, ClientFrameNr, ClientOldStatesPVSEntityIDs, ClientCurrentStateIndex, OutData);
}
