/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ServerWorld.hpp"
#include "ClientInfo.hpp"
#include "../EngineEntity.hpp"
#include "ConsoleCommands/Console.hpp"      // For cf::va().
#include "GameSys/CompHumanPlayer.hpp"
#include "GameSys/CompModel.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/World.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "../NetConst.hpp"
#include "../Common/CompGameEntity.hpp"


CaServerWorldT::CaServerWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes)
    : Ca3DEWorldT(FileName, ModelMan, GuiRes, false, NULL),
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
        if (EntityID >= m_EngineEntities.Size()) return;
        if (m_EngineEntities[EntityID] == NULL) return;

        IntrusivePtrT<cf::GameSys::EntityT> Ent = m_EngineEntities[EntityID]->GetEntity();

        if (Ent == m_ScriptWorld->GetRootEntity()) return;

        if (Ent->GetParent() != NULL)
            Ent->GetParent()->RemoveChild(Ent);

        Console->Print(cf::va("Entity %lu removed (\"%s\") in CaServerWorldT::RemoveEntity().\n", EntityID, m_EngineEntities[EntityID]->GetEntity()->GetBasics()->GetEntityName().c_str()));

        delete m_EngineEntities[EntityID];
        m_EngineEntities[EntityID]=NULL;
    }
}


unsigned long CaServerWorldT::InsertHumanPlayerEntityForNextFrame(const char* PlayerName, const char* ModelName, unsigned long ClientInfoNr)
{
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > PlayerStarts;
    IntrusivePtrT<cf::GameSys::EntityT>           PlayerPrototype = NULL;

    m_ScriptWorld->GetRootEntity()->GetAll(AllEnts);

    // Find at least one player prototype and at least one player start
    // (a single entity may act as both).
    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        if (AllEnts[EntNr]->GetComponent("PlayerStart") != NULL)
            PlayerStarts.PushBack(AllEnts[EntNr]);

        if (AllEnts[EntNr]->GetComponent("HumanPlayer") != NULL)
            if (PlayerPrototype == NULL)
                PlayerPrototype = AllEnts[EntNr];
    }

    if (PlayerStarts.Size() == 0) return 0xFFFFFFFF;
    if (PlayerPrototype == NULL)  return 0xFFFFFFFF;


    // Create a new player entity from the prototype, then initialize it.
    IntrusivePtrT<cf::GameSys::EntityT> PlayerEnt = new cf::GameSys::EntityT(*PlayerPrototype, true /*Recursive?*/);

    AllEnts.Overwrite();
    PlayerEnt->GetAll(AllEnts);

    // Filter/remove "PlayerStart" components from PlayerEnt.
    // Such a component can be useful in the player prototype, but not in the player.
    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        const ArrayT< IntrusivePtrT<cf::GameSys::ComponentBaseT> >& Components = AllEnts[EntNr]->GetComponents();

        for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
        {
            if (strcmp(Components[CompNr]->GetName(), "PlayerStart") == 0)
            {
                AllEnts[EntNr]->DeleteComponent(CompNr);
                CompNr--;
            }
        }
    }

    assert(PlayerEnt->GetComponent("PlayerStart") == NULL);

    // Set the entity name and initial transform.
    PlayerEnt->GetBasics()->SetEntityName(cf::va("Player_%lu", ClientInfoNr + 1));

    PlayerEnt->GetTransform()->SetOriginWS(PlayerStarts[0]->GetTransform()->GetOriginWS() + Vector3fT(0, 0, 40));
    PlayerEnt->GetTransform()->SetQuatWS(PlayerStarts[0]->GetTransform()->GetQuatWS());

    // Set the player's name.
    IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> HumanPlayerComp =
        dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(PlayerEnt->GetComponent("HumanPlayer"));

    HumanPlayerComp->SetMember("PlayerName", std::string(PlayerName));

    // Set the 3rd person player model.
    IntrusivePtrT<cf::GameSys::ComponentModelT> Model3rdPersonComp =
        dynamic_pointer_cast<cf::GameSys::ComponentModelT>(PlayerEnt->GetComponent("Model"));

    if (Model3rdPersonComp != NULL)   // This is optional.
        Model3rdPersonComp->SetMember("Name", std::string("Games/DeathMatch/Models/Players/") + ModelName + "/" + ModelName + ".cmdl");     // TODO... don't hardcode the path!

    // The implementation of ComponentHumanPlayerT will set the details of the collision model.
    // The inventory's maxima e.g. for bullets, shells, etc. are set by `HumanPlayer.lua`.

    // Insert the new player entity into the world.
    m_ScriptWorld->GetRootEntity()->AddChild(PlayerEnt);

    // Create matching EngineEntityT instances for PlayerEnt and all of its children.
    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        CreateNewEntityFromBasicInfo(AllEnts[EntNr], m_ServerFrameNr + 1);
    }

    // As we're inserting a new entity into a live map, post-load stuff must be run here.
    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        const ArrayT< IntrusivePtrT<cf::GameSys::ComponentBaseT> >& Components = AllEnts[EntNr]->GetComponents();

        for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
        {
            Components[CompNr]->OnPostLoad(false /*OnlyStatic?*/);
            Components[CompNr]->CallLuaMethod("OnInit", 0);
        }
    }

    return PlayerEnt->GetID();
}


void CaServerWorldT::NotifyHumanPlayerEntityOfClientCommand(unsigned long HumanPlayerEntityID, const PlayerCommandT& PlayerCommand)
{
    if (HumanPlayerEntityID < m_EngineEntities.Size())
        if (m_EngineEntities[HumanPlayerEntityID] != NULL)
        {
            IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> CompHP =
                dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(m_EngineEntities[HumanPlayerEntityID]->GetEntity()->GetComponent("HumanPlayer"));

            if (CompHP != NULL)
                CompHP->GetPlayerCommands().PushBack(PlayerCommand);
        }
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

    m_ScriptState->RunPendingCoroutines(FrameTime);   // Should do this early: new coroutines are usually added "during" thinking.

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
            CreateNewEntityFromBasicInfo(AllEnts[EntNr], m_ServerFrameNr);

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


void CaServerWorldT::UpdateFrameInfo(ClientInfoT& ClientInfo) const
{
    const EngineEntityT* EE = m_EngineEntities[ClientInfo.EntityID];

    if (!EE) return;

    const cf::SceneGraph::BspTreeNodeT* BspTree = m_World->m_StaticEntityData[0]->m_BspTree;
    ArrayT<unsigned long>& NewStatePVSEntityIDs = ClientInfo.OldStatesPVSEntityIDs[m_ServerFrameNr & (ClientInfo.OldStatesPVSEntityIDs.Size() - 1)];
    const unsigned long    ClientLeafNr         = BspTree->WhatLeaf(EE->GetEntity()->GetTransform()->GetOriginWS().AsVectorOfDouble());

    NewStatePVSEntityIDs.Overwrite();

    // Determine all entities that are relevant for (in the PVS of) this client.
    for (unsigned long EntityNr = 0; EntityNr < m_EngineEntities.Size(); EntityNr++)
        if (m_EngineEntities[EntityNr] != NULL)
        {
            BoundingBox3dT EntityBB = m_EngineEntities[EntityNr]->GetEntity()->GetCullingBB(true /*WorldSpace*/).AsBoxOfDouble();

            if (!EntityBB.IsInited())
            {
                // If the entity has no visual representation, add its origin point in order to "compensate" this.
                // At this time, accounting for such "invisible" entities is useful, because e.g. we have no explicit
                // "potentially audible set" for sound sources. This will also cover entities that the client *really*
                // cannot see, e.g. player starting points and other purely informational entities, but that's ok for now.
                EntityBB += m_EngineEntities[EntityNr]->GetEntity()->GetTransform()->GetOriginWS().AsVectorOfDouble();
            }

            if (BspTree->IsInPVS(EntityBB, ClientLeafNr)) NewStatePVSEntityIDs.PushBack(EntityNr);
        }

    // Make sure that NewStatePVSEntityIDs is in sorted order. At the time of this writing, this is trivially the
    // case, but it is also easy to foresee changes to the above loop that unintentionally break this rule, which is
    // an important requirement for the code below and whose violations may cause hard to diagnose problems.
    for (unsigned int i = 1; i < NewStatePVSEntityIDs.Size(); i++)
        assert(NewStatePVSEntityIDs[i - 1] < NewStatePVSEntityIDs[i]);
}


void CaServerWorldT::WriteClientDeltaUpdateMessages(const ClientInfoT& ClientInfo, NetDataT& OutData) const
{
    const unsigned long ClientFrameNr = ClientInfo.LastKnownFrameReceived;
    const ArrayT<unsigned long>* NewStatePVSEntityIDs = &ClientInfo.OldStatesPVSEntityIDs[m_ServerFrameNr & (ClientInfo.OldStatesPVSEntityIDs.Size() - 1)];
    const ArrayT<unsigned long>* OldStatePVSEntityIDs = NULL;

    unsigned long DeltaFrameNr;     // Kann dies entfernen, indem der Packet-Header direkt im if-else-Teil geschrieben wird!

    if (ClientFrameNr == 0 || ClientFrameNr >= m_ServerFrameNr || ClientFrameNr + ClientInfo.OldStatesPVSEntityIDs.Size() - 1 < m_ServerFrameNr)
    {
        // Erläuterung der obigen if-Bedingung:
        // a) Der erste  Teil 'ClientFrameNr==0' ist klar! (Echt?? Vermutlich war gemeint, dass der Client in der letzten CS1_FrameInfoACK Nachricht explizit "0" geschickt und damit Baseline angefordert hat.)
        // b) Der zweite Teil 'ClientFrameNr>=ServerFrameNr' ist nur zur Sicherheit und sollte NIEMALS anspringen!
        // c) Der dritte Teil ist äquivalent zu 'ServerFrameNr-ClientFrameNr>=ClientOldStatesPVSEntityIDs.Size()'!
        static const ArrayT<unsigned long> EmptyArray;

        // Entweder will der Client explizit ein retransmit haben (bei neuer World oder auf User-Wunsch (no-delta mode) oder nach Problemen),
        // oder beim Client ist schon länger keine verwertbare Nachricht mehr angekommen. Daher delta'en wir bzgl. der BaseLine!
        DeltaFrameNr         = 0;
        OldStatePVSEntityIDs = &EmptyArray;
    }
    else
    {
        DeltaFrameNr         = ClientFrameNr;
        OldStatePVSEntityIDs = &ClientInfo.OldStatesPVSEntityIDs[ClientFrameNr & (ClientInfo.OldStatesPVSEntityIDs.Size() - 1)];
    }


    OutData.WriteByte(SC1_FrameInfo);
    OutData.WriteLong(m_ServerFrameNr);     // What we are delta'ing to   (Frame, für das wir Informationen schicken)
    OutData.WriteLong(DeltaFrameNr);        // What we are delta'ing from (Frame, auf das wir uns beziehen (0 für BaseLine))
    OutData.WriteLong(ClientInfo.LastPlayerCommandNr);  // The number of the last player command that has been received (and accounted for in m_ServerFrameNr).


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
        // (Also see constant GameProtocol1T::MAX_MSG_SIZE that defines the maximum package size.)
        const bool SkipEntity = OutData.Data.Size() > 4096 && NewEntityID != ClientInfo.EntityID;

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
