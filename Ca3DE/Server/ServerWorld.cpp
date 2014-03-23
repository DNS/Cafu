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

#include "ServerWorld.hpp"
#include "../EngineEntity.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "ConsoleCommands/Console.hpp"      // For cf::va().
#include "GameSys/Entity.hpp"
#include "GameSys/EntityCreateParams.hpp"
#include "GameSys/World.hpp"
#include "Network/Network.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "TypeSys.hpp"
#include "../NetConst.hpp"
#include "../Common/CompGameEntity.hpp"


CaServerWorldT::CaServerWorldT(cf::GameSys::GameInfoI* GameInfo, cf::GameSys::GameI* Game, const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes)
    : Ca3DEWorldT(GameInfo, Game, FileName, ModelMan, GuiRes, false, NULL),
      // Note that 0 is reserved for referring to the "baseline" (the state in which entities were created),
      // as opposed to the entity state at a specific server frame.
      // (The `ClientInfoT::LastKnownFrameReceived` start at 0, see ClientInfoT::InitForNewWorld() for details.)
      m_ServerFrameNr(1),
      m_IsThinking(false),
      m_EntityRemoveList()
{
    // Note that we must NOT modify anything about the entity states here --
    // all entity states at frame 1 must be EXACT matches on the client and the server!
}


unsigned long CaServerWorldT::CreateNewEntity(const std::map<std::string, std::string>& Properties, unsigned long CreationFrameNr, const VectorT& Origin)
{
    IntrusivePtrT<cf::GameSys::EntityT> NewEnt  = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));
    IntrusivePtrT<CompGameEntityT>      GameEnt = new CompGameEntityT();

    NewEnt->GetTransform()->SetOriginWS(Origin.AsVectorOfFloat());
    NewEnt->SetApp(GameEnt);
    m_ScriptWorld->GetRootEntity()->AddChild(NewEnt);

    GameEnt->GetStaticEntityData()->m_Properties = Properties;

    return CreateNewEntityFromBasicInfo(GameEnt, CreationFrameNr);
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
            Console->Print(cf::va("Entity %lu removed (\"%s\") in CaServerWorldT::RemoveEntity().\n", EntityID, m_EngineEntities[EntityID]->GetEntity()->GetBasics()->GetEntityName().c_str()));
            delete m_EngineEntities[EntityID];
            m_EngineEntities[EntityID]=NULL;
        }
    }
}


unsigned long CaServerWorldT::InsertHumanPlayerEntityForNextFrame(const char* PlayerName, const char* ModelName, unsigned long ClientInfoNr)
{
    IntrusivePtrT<cf::GameSys::EntityT> NewEnt  = new cf::GameSys::EntityT(cf::GameSys::EntityCreateParamsT(*m_ScriptWorld));
    IntrusivePtrT<CompGameEntityT>      GameEnt = new CompGameEntityT();

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
        if (AllEnts[EntNr]->GetComponent("PlayerStart") != NULL)
        {
            NewEnt->GetTransform()->SetOriginWS(AllEnts[EntNr]->GetTransform()->GetOriginWS() + Vector3fT(0, 0, 40));
            NewEnt->GetTransform()->SetQuatWS(AllEnts[EntNr]->GetTransform()->GetQuatWS());
            break;
        }

    NewEnt->GetBasics()->SetEntityName(cf::va("Player_%lu", ClientInfoNr+1));
    NewEnt->SetApp(GameEnt);
    m_ScriptWorld->GetRootEntity()->AddChild(NewEnt);

    GameEnt->GetStaticEntityData()->m_Properties["classname"] = "HumanPlayer";
    GameEnt->GetStaticEntityData()->m_Properties["name"]      = cf::va("Player%lu", ClientInfoNr+1);     // Setting the name is needed so that player entities can have a corresponding script instance.
 // GameEnt->GetStaticEntityData()->m_Properties["angles"]    = cf::va("0 %lu 0", (unsigned long)(m_World->InfoPlayerStarts[0].Heading/8192.0*45.0));

    return CreateNewEntityFromBasicInfo(GameEnt, m_ServerFrameNr+1, PlayerName, ModelName);
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

    // Must never move this above the PreThink() calls above, because ...(?)
    m_PhysicsWorld.Think(FrameTime);

    m_ScriptWorld->GetScriptState().RunPendingCoroutines(FrameTime);    // Should do this early: new coroutines are usually added "during" thinking.

    for (unsigned long EntityNr=0; EntityNr<m_EngineEntities.Size(); EntityNr++)
        if (m_EngineEntities[EntityNr]!=NULL)
            m_EngineEntities[EntityNr]->Think(FrameTime, m_ServerFrameNr);

    m_IsThinking=false;


    // If entities removed other entities (or even themselves!) while thinking, remove them now.
    for (unsigned long EntNr = 0; EntNr < m_EngineEntities.Size(); EntNr++)
    {
        if (m_EngineEntities[EntNr] == NULL) continue;

        IntrusivePtrT<cf::GameSys::EntityT> Ent = m_EngineEntities[EntNr]->GetEntity();

        if (Ent != m_ScriptWorld->GetRootEntity() && Ent->GetParent().IsNull())
        {
            Console->Print(cf::va("Entity %lu removed (\"%s\"), it no longer has a parent.\n", EntNr, m_EngineEntities[EntNr]->GetEntity()->GetBasics()->GetEntityName().c_str()));
            delete m_EngineEntities[EntNr];
            m_EngineEntities[EntNr]=NULL;
        }
    }

    for (unsigned long RemoveNr=0; RemoveNr<m_EntityRemoveList.Size(); RemoveNr++)
    {
        const unsigned long EntityID=m_EntityRemoveList[RemoveNr];

        Console->Print(cf::va("Entity %lu removed (\"%s\") via m_EntityRemoveList.\n", EntityID, m_EngineEntities[EntityID]->GetEntity()->GetBasics()->GetEntityName().c_str()));
        delete m_EngineEntities[EntityID];
        m_EngineEntities[EntityID]=NULL;
    }

    m_EntityRemoveList.Overwrite();


    // If entities (script code) created new entities:
    //   - they only exist in the script world, but not yet in m_EngineEntities,
    //   - they have no App component, which we can use to identify them.
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;

    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        if (AllEnts[EntNr]->GetApp() == NULL)
        {
            IntrusivePtrT<CompGameEntityT> GameEnt = new CompGameEntityT();

            AllEnts[EntNr]->SetApp(GameEnt);
            CreateNewEntityFromBasicInfo(GameEnt, m_ServerFrameNr);

            const ArrayT< IntrusivePtrT<cf::GameSys::ComponentBaseT> >& Components = AllEnts[EntNr]->GetComponents();

            for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
            {
                // The components belong to an entity that was newly added to a live map.
                // Consequently, we must run the post-load stuff here.
                Components[CompNr]->OnPostLoad(false);
                Components[CompNr]->CallLuaMethod("OnInit", 0);
            }
        }
    }
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


    const cf::SceneGraph::BspTreeNodeT* BspTree = m_World->m_StaticEntityData[0]->m_BspTree;

    ArrayT<unsigned long>* NewStatePVSEntityIDs=&ClientOldStatesPVSEntityIDs[ClientCurrentStateIndex];
    ArrayT<unsigned long>* OldStatePVSEntityIDs=NULL;
    const unsigned long    ClientLeafNr        =(m_EngineEntities[ClientEntityID]!=NULL) ? BspTree->WhatLeaf(m_EngineEntities[ClientEntityID]->GetGameEntity()->GetOrigin()) : 0;

    // Finde heraus, welche Entities im PVS von diesem Client liegen. Erhalte ein Array von EntityIDs.
    for (unsigned long EntityNr=0; EntityNr<m_EngineEntities.Size(); EntityNr++)
        if (m_EngineEntities[EntityNr]!=NULL)
        {
            const Vector3dT&      EntityOrigin=m_EngineEntities[EntityNr]->GetGameEntity()->GetOrigin();
            BoundingBox3T<double> EntityBB    =m_EngineEntities[EntityNr]->GetGameEntity()->GetDimensions();

            EntityBB.Min += EntityOrigin;
            EntityBB.Max += EntityOrigin;

            if (BspTree->IsInPVS(EntityBB, ClientLeafNr)) NewStatePVSEntityIDs->PushBack(EntityNr);
        }


    unsigned long DeltaFrameNr;     // Kann dies entfernen, indem der Packet-Header direkt im if-else-Teil geschrieben wird!

    if (ClientFrameNr == 0 || ClientFrameNr >= m_ServerFrameNr || ClientFrameNr+ClientOldStatesPVSEntityIDs.Size()-1 < m_ServerFrameNr)
    {
        // Erläuterung der obigen if-Bedingung:
        // a) Der erste  Teil 'ClientFrameNr==0' ist klar! (Echt?? Vermutlich war gemeint, dass der Client in der letzten CS1_FrameInfoACK Nachricht explizit "0" geschickt und damit Baseline angefordert hat.)
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


    unsigned long OldIndex = 0;
    unsigned long NewIndex = 0;

    while (OldIndex < OldStatePVSEntityIDs->Size() || NewIndex < NewStatePVSEntityIDs->Size())
    {
        const unsigned long OldEntityID = OldIndex < OldStatePVSEntityIDs->Size() ? (*OldStatePVSEntityIDs)[OldIndex] : 0x99999999;
        const unsigned long NewEntityID = NewIndex < NewStatePVSEntityIDs->Size() ? (*NewStatePVSEntityIDs)[NewIndex] : 0x99999999;

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
        const bool SkipEntity = OutData.Data.Size() > 1250 && NewEntityID != ClientEntityID;

        if (OldEntityID == NewEntityID)
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

        if (OldEntityID > NewEntityID)
        {
            // Dies ist ein neuer Entity, sende ihn von der BaseLine aus.
            // Deswegen kann der folgende Aufruf (gemäß der Spezifikation von WriteDeltaEntity()) auch nicht scheitern!
            if (!SkipEntity)
                m_EngineEntities[NewEntityID]->WriteDeltaEntity(true /* send from baseline? */, 0, OutData, true);

            NewIndex++;
            continue;
        }

        if (OldEntityID < NewEntityID)
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
