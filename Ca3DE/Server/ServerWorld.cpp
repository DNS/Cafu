/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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
#include "../Both/EngineEntity.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/Console.hpp"      // For cf::va().
#include "Network/Network.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "../NetConst.hpp"
#include "../../Games/BaseEntity.hpp"
#include "../../Games/Game.hpp"


static ConVarT AutoAddCompanyBot("sv_AutoAddCompanyBot", false, ConVarT::FLAG_MAIN_EXE, "If true, auto-inserts an entity of type \"Company Bot\" at the player start position into each newly loaded map.");


CaServerWorldT::CaServerWorldT(const char* FileName, ModelManagerT& ModelMan)
    : Ca3DEWorldT(FileName, ModelMan, false, NULL),
      m_ServerFrameNr(1),   // 0 geht nicht, denn die ClientInfoT::LastKnownFrameReceived werden mit 0 initialisiert!
      m_IsThinking(false),
      m_EntityRemoveList()
{
    cf::GameSys::Game->Sv_PrepareNewWorld(FileName, m_World->CollModel);

    // Gehe alle GameEntities der Ca3DEWorld durch und erstelle dafür "echte" Entities.
    for (unsigned long GENr=0; GENr<m_World->GameEntities.Size(); GENr++)
    {
        const GameEntityT* GE=m_World->GameEntities[GENr];

        // Register GE->CollModel also with the cf::ClipSys::CollModelMan, so that both the owner (the game entity GE)
        // as well as the game code can free/delete it in their destructors (one by "delete", the other by cf::ClipSys::CollModelMan->FreeCM()).
        cf::ClipSys::CollModelMan->GetCM(GE->CollModel);

        CreateNewEntityFromBasicInfo(GE->Properties, GE->BspTree, GE->CollModel, GENr, GE->MFIndex, m_ServerFrameNr, GE->Origin);
    }

    // Gehe alle InfoPlayerStarts der Ca3DEWorld durch und erstelle dafür "echte" Entities.
    for (unsigned long IPSNr=0; IPSNr<m_World->InfoPlayerStarts.Size(); IPSNr++)
    {
        std::map<std::string, std::string> Props;

        Props["classname"]="info_player_start";
        Props["angles"]   =cf::va("0 %lu 0", (unsigned long)(m_World->InfoPlayerStarts[IPSNr].Heading/8192.0*45.0));

        CreateNewEntityFromBasicInfo(Props, NULL, NULL, (unsigned long)-1, (unsigned long)-1, m_ServerFrameNr, m_World->InfoPlayerStarts[IPSNr].Origin);
    }

    // Zu Demonstrationszwecken fügen wir auch noch einen MonsterMaker vom Typ CompanyBot in die World ein.
    // TODO! Dies sollte ins Map/Entity-Script wandern!!!
    if (AutoAddCompanyBot.GetValueBool())
    {
        std::map<std::string, std::string> Props;

        Props["classname"]         ="LifeFormMaker";
        Props["angles"]            =cf::va("0 %lu 0", (unsigned long)(m_World->InfoPlayerStarts[0].Heading/8192.0*45.0));
        Props["monstertype"]       ="CompanyBot";
        Props["monstercount"]      ="1";
        Props["m_imaxlivechildren"]="1";

        CreateNewEntityFromBasicInfo(Props, NULL, NULL, (unsigned long)-1, (unsigned long)-1, m_ServerFrameNr, m_World->InfoPlayerStarts[0].Origin);
    }

    cf::GameSys::Game->Sv_FinishNewWorld(FileName);
}


CaServerWorldT::~CaServerWorldT()
{
    Clear();                                // Unregisters all entities from the games script state.
    cf::GameSys::Game->Sv_UnloadWorld();    // Deletes the games script state.
}


unsigned long CaServerWorldT::CreateNewEntity(const std::map<std::string, std::string>& Properties, unsigned long CreationFrameNr, const VectorT& Origin)
{
    return CreateNewEntityFromBasicInfo(Properties, NULL, NULL, (unsigned long)(-1), (unsigned long)(-1), CreationFrameNr, Origin);
}


// Die Clients bekommen unabhängig hiervon in einer SC1_DropClient Message explizit mitgeteilt, wenn ein Client (warum auch immer) den Server verläßt.
// Den dazugehörigen Entity muß der Client deswegen aber nicht unbedingt sofort und komplett aus seiner World entfernen,
// dies sollte vielmehr durch Wiederverwendung von EntityIDs durch den Server geschehen!
void CaServerWorldT::RemoveEntity(unsigned long EntityID)
{
    if (m_IsThinking)
    {
        // We're currently thinking, and EntityID might be the ID of the entity that currently thinks.
        // (That is, this entity is removing itself, as for example an exploded grenade.)
        // Thus, schedule this entity for removal until the thinking is finished.
        m_EntityRemoveList.PushBack(EntityID);
    }
    else
    {
        // Currently not thinking, so it should be safe to remove the entity immediately.
        if (EntityID < m_EngineEntities.Size())
        {
            delete m_EngineEntities[EntityID];
            m_EngineEntities[EntityID]=NULL;
        }
    }
}


unsigned long CaServerWorldT::InsertHumanPlayerEntityForNextFrame(const char* PlayerName, const char* ModelName, unsigned long ClientInfoNr)
{
    std::map<std::string, std::string> Props;

    Props["classname"]="HumanPlayer";
    Props["name"]     =cf::va("Player%lu", ClientInfoNr+1);     // Setting the name is needed so that player entities can have a corresponding script instance.
    Props["angles"]   =cf::va("0 %lu 0", (unsigned long)(m_World->InfoPlayerStarts[0].Heading/8192.0*45.0));

    return CreateNewEntityFromBasicInfo(Props, NULL, NULL, (unsigned long)-1, (unsigned long)-1, m_ServerFrameNr+1, m_World->InfoPlayerStarts[0].Origin+VectorT(0, 0, 1000), PlayerName, ModelName);
}


void CaServerWorldT::NotifyHumanPlayerEntityOfClientCommand(unsigned long HumanPlayerEntityID, const PlayerCommandT& PlayerCommand)
{
    if (HumanPlayerEntityID < m_EngineEntities.Size())
        if (m_EngineEntities[HumanPlayerEntityID]!=NULL)
            m_EngineEntities[HumanPlayerEntityID]->ProcessConfigString(&PlayerCommand, "PlayerCommand");
}


void CaServerWorldT::Think(float FrameTime)
{
    // Zuerst die Nummer des nächsten Frames 'errechnen'.
    // Die Reihenfolge ist wichtig, denn wenn ein neuer Entity geschaffen wird,
    // muß dieser korrekt wissen, zu welchem Frame er ins Leben gerufen wurde.
    m_ServerFrameNr++;

    // Jetzt das eigentliche Denken durchführen.
    // Heraus kommt eine Aussage der Form: "Zum Frame Nummer 'm_ServerFrameNr' ist die World in diesem Zustand!"
    if (m_IsThinking) return;

    m_IsThinking=true;

    // Beachte:
    // - Neu geschaffene Entities sollen nicht gleich 'Think()'en!
    //   Zur Erreichung vergleiche dazu die Implementation von EngineEntityT::Think().
    //   DÜRFTEN sie trotzdem gleich Think()en??? (JA!) Die OldStates kämen dann evtl. durcheinander!? (NEIN!)
    //   Allerdings übertragen wir mit BaseLines grundsätzlich KEINE Events (??? PRÜFEN!), Think()en macht insofern also nur eingeschränkt Sinn.
    // - EntityIDs sollten wohl besser NICHT wiederverwendet werden, da z.B. Parents die IDs ihrer Children speichern usw.
    // - Letzteres führt aber zu zunehmend vielen NULL-Pointern im m_EngineEntities-Array.
    // - Dies könnte sich evtl. mit einem weiteren Array von 'active EntityIDs' lösen lassen.
    for (unsigned long EntityNr=0; EntityNr<m_EngineEntities.Size(); EntityNr++)
        if (m_EngineEntities[EntityNr]!=NULL)
            m_EngineEntities[EntityNr]->PreThink(m_ServerFrameNr);

    // Must never move this above the PreThink() calls above, because the Game assumes that the entity states may
    // be modified (e.g. by map script commands) as soon as it gets this call.
    cf::GameSys::Game->Sv_BeginThinking(FrameTime);

    for (unsigned long EntityNr=0; EntityNr<m_EngineEntities.Size(); EntityNr++)
        if (m_EngineEntities[EntityNr]!=NULL)
            m_EngineEntities[EntityNr]->Think(FrameTime, m_ServerFrameNr);

    cf::GameSys::Game->Sv_EndThinking();

    m_IsThinking=false;


    // If entities removed other entities (or even themselves!) while thinking, remove them now.
    for (unsigned long RemoveNr=0; RemoveNr<m_EntityRemoveList.Size(); RemoveNr++)
    {
        const unsigned long EntityID=m_EntityRemoveList[RemoveNr];

        delete m_EngineEntities[EntityID];
        m_EngineEntities[EntityID]=NULL;
    }

    m_EntityRemoveList.Overwrite();
}


unsigned long CaServerWorldT::WriteClientNewBaseLines(unsigned long OldBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const
{
    const unsigned long SentClientBaseLineFrameNr = OldBaseLineFrameNr;

    for (unsigned long EntityNr=0; EntityNr < m_EngineEntities.Size(); EntityNr++)
        if (m_EngineEntities[EntityNr]!=NULL)
            m_EngineEntities[EntityNr]->WriteNewBaseLine(SentClientBaseLineFrameNr, OutDatas);

    return m_ServerFrameNr;
}


void CaServerWorldT::WriteClientDeltaUpdateMessages(unsigned long ClientEntityID, unsigned long ClientFrameNr, ArrayT< ArrayT<unsigned long> >& ClientOldStatesPVSEntityIDs, unsigned long& ClientCurrentStateIndex, NetDataT& OutData) const
{
    // Wenn dies hier aufgerufen wird, befinden sich sämtliche m_EngineEntities schon im Zustand ('Entity->State') zum Frame 'ServerFrameNr'.
    // Der Client, von dem obige Parameter stammen, ist aber noch nicht soweit (sondern noch im vorherigen Zustand).
    // Update daher zuerst die PVS-EntityID Infos dieses Clients.
    const char TEMP_MAX_OLDSTATES=16+1;     // (?)

    if (ClientOldStatesPVSEntityIDs.Size()<TEMP_MAX_OLDSTATES)
    {
        ClientOldStatesPVSEntityIDs.PushBackEmpty();

        ClientCurrentStateIndex=ClientOldStatesPVSEntityIDs.Size()-1;
    }
    else
    {
        ClientCurrentStateIndex++;
        if (ClientCurrentStateIndex>=TEMP_MAX_OLDSTATES) ClientCurrentStateIndex=0;

        ClientOldStatesPVSEntityIDs[ClientCurrentStateIndex].Clear();
    }


    ArrayT<unsigned long>* NewStatePVSEntityIDs=&ClientOldStatesPVSEntityIDs[ClientCurrentStateIndex];
    ArrayT<unsigned long>* OldStatePVSEntityIDs=NULL;
    unsigned long          ClientLeafNr        =(m_EngineEntities[ClientEntityID]!=NULL) ? m_World->BspTree->WhatLeaf(m_EngineEntities[ClientEntityID]->GetBaseEntity()->GetOrigin()) : 0;

    // Finde heraus, welche Entities im PVS von diesem Client liegen. Erhalte ein Array von EntityIDs.
    for (unsigned long EntityNr=0; EntityNr<m_EngineEntities.Size(); EntityNr++)
        if (m_EngineEntities[EntityNr]!=NULL)
        {
            const Vector3dT&      EntityOrigin=m_EngineEntities[EntityNr]->GetBaseEntity()->GetOrigin();
            BoundingBox3T<double> EntityBB    =m_EngineEntities[EntityNr]->GetBaseEntity()->GetDimensions();

            EntityBB.Min += EntityOrigin;
            EntityBB.Max += EntityOrigin;

            if (m_World->BspTree->IsInPVS(EntityBB, ClientLeafNr)) NewStatePVSEntityIDs->PushBack(EntityNr);
        }


    unsigned long DeltaFrameNr;     // Kann dies entfernen, indem der Packet-Header direkt im if-else-Teil geschrieben wird!

    if (ClientFrameNr==0 || ClientFrameNr>=m_ServerFrameNr || ClientFrameNr+ClientOldStatesPVSEntityIDs.Size()-1<m_ServerFrameNr)
    {
        // Erläuterung der obigen if-Bedingung:
        // a) Der erste  Teil 'ClientFrameNr==0' ist klar!
        // b) Der zweite Teil 'ClientFrameNr>=ServerFrameNr' ist nur zur Sicherheit und sollte NIEMALS anspringen!
        // c) Der dritte Teil ist äquivalent zu 'ServerFrameNr-ClientFrameNr>=ClientOldStatesPVSEntityIDs.Size()'!
        static ArrayT<unsigned long> EmptyArray;

        // Entweder will der Client explizit ein retransmit haben (bei neuer World oder auf User-Wunsch (no-delta mode) oder nach Problemen),
        // oder beim Client ist schon länger keine verwertbare Nachricht mehr angekommen. Daher delta'en wir bzgl. der BaseLine!
        DeltaFrameNr        =0;
        OldStatePVSEntityIDs=&EmptyArray;
    }
    else
    {
        // Nach obiger if-Bedingung ist FrameDiff auf jeden Fall in [1 .. ClientOldStatesPVSEntityIDs.Size()-1].
        unsigned long FrameDiff=m_ServerFrameNr-ClientFrameNr;

        DeltaFrameNr        =ClientFrameNr;
        OldStatePVSEntityIDs=&ClientOldStatesPVSEntityIDs[FrameDiff<=ClientCurrentStateIndex ? ClientCurrentStateIndex-FrameDiff : ClientOldStatesPVSEntityIDs.Size()+ClientCurrentStateIndex-FrameDiff];
    }


    OutData.WriteByte(SC1_FrameInfo);
    OutData.WriteLong(m_ServerFrameNr);     // What we are delta'ing to   (Frame, für das wir Informationen schicken)
    OutData.WriteLong(DeltaFrameNr);        // What we are delta'ing from (Frame, auf das wir uns beziehen (0 für BaseLine))


    unsigned long OldIndex=0;
    unsigned long NewIndex=0;

    while (OldIndex<OldStatePVSEntityIDs->Size() || NewIndex<NewStatePVSEntityIDs->Size())
    {
        unsigned long OldEntityID=OldIndex<OldStatePVSEntityIDs->Size() ? (*OldStatePVSEntityIDs)[OldIndex] : 0x99999999;
        unsigned long NewEntityID=NewIndex<NewStatePVSEntityIDs->Size() ? (*NewStatePVSEntityIDs)[NewIndex] : 0x99999999;

        // Consider the following situation:
        // There are (or were) A REALLY BIG NUMBER of entities in the PVS of the current client entity ('m_EngineEntities[ClientEntityID]').
        // Each of them would cause data to be written into 'OutData'.
        // Unfortunately, if the network protocol detects later that 'OutData.Data.Size()' exceeds the maximum possible size,
        // the entire content is dropped, as it is only considered "unreliable data".
        // That's not inherently dangerous, BUT it tends to happen repeatedly (assuming the client does not move into another leaf with fewer entities).
        // Therefore, the client gets *no* updates anymore, and the truly problematic fact is that its *own* update is among those that get dropped!
        // The client-side prediction continues to let the client move for a while, but eventually it gets stuck.
        // (Theoretically, the client could try to move "blindly" into a leaf with few enough entities such that updates get through again
        //  (server operation in not tampered by this condition!), but until then, the screen seems to indicate that it is stuck/frozen.)
        // Okay, but what can we do? Carefully think about it, and you will find that NOTHING can be done without introducing NEW problems!
        // Why? The spec. / code simply requires the consistency of this function! ANY changes here cause trouble with delta updates later!
        // It seems that the only reasonable solution is to omit update messages of "other" entities:
        // Better risk not-updated fields of other entities, than getting stuck with the own entity!
        // In order to achieve this, the introduction of the 'SkipEntity' variable is the best solution I could think of.
        bool SkipEntity=OutData.Data.Size()>1250 && NewEntityID!=ClientEntityID;

        if (OldEntityID==NewEntityID)
        {
            // Diesen Entity gab es schon im alten Frame.
            // Hierhin kommen wir nur, wenn 'OldStatePVSEntityIDs' nicht leer ist, vergleiche mit obigem Code!
            // PRINZIPIELL geht dann die Differenz 'ServerFrameNr-ClientFrameNr' in Ordnung, ebenfalls nach obigem Code.
            // TATSÄCHLICH wäre noch denkbar, daß der Entity mit ID 'NewEntityID' erst neu erschaffen wurde,
            // d.h. jünger ist als der Client, und deshalb die Differenz doch zu groß ist!
            // Dies wird aber von der Logik hier vermieden, denn ein solcher neuer Entity kann ja nicht schon im alten Frame vorgekommen sein!
            // Mit anderen Worten: Der folgende Aufruf sollte NIEMALS scheitern, falls doch, ist das ein fataler Fehler, der intensives Debugging erfordert.
            // Dennoch ist es wahrscheinlich (??) nicht notwendig, den Client bei Auftreten dieses Fehler zu disconnecten.
            if (!SkipEntity)
                if (!m_EngineEntities[NewEntityID]->WriteDeltaEntity(false /* send from baseline? */, ClientFrameNr, OutData, false))
                    EnqueueString("SERVER ERROR: %s, L %u: NewEntityID %u, ServerFrameNr %u, ClientFrameNr %u\n", __FILE__, __LINE__, NewEntityID, m_ServerFrameNr, ClientFrameNr);

            OldIndex++;
            NewIndex++;
            continue;
        }

        if (OldEntityID>NewEntityID)
        {
            // Dies ist ein neuer Entity, sende ihn von der BaseLine aus.
            // Deswegen kann der folgende Aufruf (gemäß der Spezifikation von WriteDeltaEntity()) auch nicht scheitern!
            if (!SkipEntity)
                m_EngineEntities[NewEntityID]->WriteDeltaEntity(true /* send from baseline? */, 0, OutData, true);

            NewIndex++;
            continue;
        }

        if (OldEntityID<NewEntityID)
        {
            // Diesen Entity gibt es im neuen Frame nicht mehr.
            if (!SkipEntity)
            {
                OutData.WriteByte(SC1_EntityRemove);
                OutData.WriteLong(OldEntityID);
            }

            OldIndex++;
            continue;
        }
    }
}


unsigned long CaServerWorldT::CreateNewEntityFromBasicInfo(const std::map<std::string, std::string>& Properties,
    const cf::SceneGraph::GenericNodeT* RootNode, const cf::ClipSys::CollisionModelT* CollisionModel,
    unsigned long WorldFileIndex, unsigned long MapFileIndex, unsigned long CreationFrameNr, const VectorT& Origin, const char* PlayerName, const char* ModelName)
{
    unsigned long NewEntityID  =m_EngineEntities.Size();
    BaseEntityT*  NewBaseEntity=cf::GameSys::Game->CreateBaseEntityFromMapFile(Properties, RootNode, CollisionModel, NewEntityID,
                                    WorldFileIndex, MapFileIndex, this, Origin);

    if (NewBaseEntity)
    {
        // Muß dies VOR dem Erzeugen des EngineEntitys tun, denn sonst stimmt dessen BaseLine nicht!
        if (PlayerName!=NULL) NewBaseEntity->ProcessConfigString(PlayerName, "PlayerName");
        if (ModelName !=NULL) NewBaseEntity->ProcessConfigString(ModelName , "ModelName" );

        m_EngineEntities.PushBack(new EngineEntityT(NewBaseEntity, CreationFrameNr));
        return NewEntityID;
    }

    // Free the collision model in place of the (never instantiated) entity destructor,
    // so that the reference count of the CollModelMan gets right.
    cf::ClipSys::CollModelMan->FreeCM(CollisionModel);

    return 0xFFFFFFFF;  // Fehlerwert zurückgeben
}
