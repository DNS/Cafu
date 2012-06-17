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

#include "EntityManager.hpp"
#include "EngineEntity.hpp"
#include "Ca3DEWorld.hpp"
#include "../NetConst.hpp"
#include "Network/Network.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "ClipSys/CollisionModel_static.hpp"
#include "ClipSys/CollisionModelMan.hpp"
#include "SceneGraph/BspTreeNode.hpp"
#include "../../Games/BaseEntity.hpp"
#include "../../Games/Game.hpp"


/******************/
/*** Both Sides ***/
/******************/


EntityManagerT::EntityManagerT(Ca3DEWorldT& Ca3DEWorld_)
    : Ca3DEWorld(Ca3DEWorld_),
      IsThinking(false)
{
}


EntityManagerT::~EntityManagerT()
{
    for (unsigned long EntityNr=0; EntityNr<EngineEntities.Size(); EntityNr++)
        delete EngineEntities[EntityNr];
}


void EntityManagerT::GetAllEntityIDs(ArrayT<unsigned long>& EntityIDs) const
{
    for (unsigned long EntityNr=0; EntityNr<EngineEntities.Size(); EntityNr++)
        if (EngineEntities[EntityNr]!=NULL)
            EntityIDs.PushBack(EntityNr);
}


BaseEntityT* EntityManagerT::GetBaseEntityByID(unsigned long EntityID) const
{
    if (EntityID<EngineEntities.Size())
        if (EngineEntities[EntityID]!=NULL)
            return EngineEntities[EntityID]->GetBaseEntity();

    return NULL;
}


void EntityManagerT::ProcessConfigString(unsigned long EntityID, const void* ConfigData, const char* ConfigString)
{
    if (EntityID<EngineEntities.Size())
        if (EngineEntities[EntityID]!=NULL)
            EngineEntities[EntityID]->ProcessConfigString(ConfigData, ConfigString);
}


/*******************/
/*** Server Side ***/
/*******************/


unsigned long EntityManagerT::CreateNewEntityFromBasicInfo(const std::map<std::string, std::string>& Properties,
    const cf::SceneGraph::GenericNodeT* RootNode, const cf::ClipSys::CollisionModelT* CollisionModel,
    unsigned long WorldFileIndex, unsigned long MapFileIndex, unsigned long CreationFrameNr, const VectorT& Origin, const char* PlayerName, const char* ModelName)
{
    unsigned long NewEntityID  =EngineEntities.Size();
    BaseEntityT*  NewBaseEntity=cf::GameSys::Game->CreateBaseEntityFromMapFile(Properties, RootNode, CollisionModel, NewEntityID,
                                    WorldFileIndex, MapFileIndex, &Ca3DEWorld, Origin);

    if (NewBaseEntity)
    {
        // Muß dies VOR dem Erzeugen des EngineEntitys tun, denn sonst stimmt dessen BaseLine nicht!
        if (PlayerName!=NULL) NewBaseEntity->ProcessConfigString(PlayerName, "PlayerName");
        if (ModelName !=NULL) NewBaseEntity->ProcessConfigString(ModelName , "ModelName" );

        EngineEntities.PushBack(new EngineEntityT(NewBaseEntity, CreationFrameNr));
        return NewEntityID;
    }

    // Free the collision model in place of the (never instantiated) entity destructor,
    // so that the reference count of the CollModelMan gets right.
    cf::ClipSys::CollModelMan->FreeCM(CollisionModel);

    return 0xFFFFFFFF;  // Fehlerwert zurückgeben
}


void EntityManagerT::RemoveEntity(unsigned long EntityID)
{
    if (IsThinking)
    {
        // We're currently thinking, and EntityID might be the ID of the entity that currently thinks.
        // (That is, this entity is removing itself, as for example an exploded grenade.)
        // Thus, schedule this entity for removal until the thinking is finished.
        EntityRemoveList.PushBack(EntityID);
    }
    else
    {
        // Currently not thinking, so it should be save to remove the entity immediately.
        if (EntityID<EngineEntities.Size())
        {
            delete EngineEntities[EntityID];
            EngineEntities[EntityID]=NULL;
        }
    }
}


void EntityManagerT::Think(float FrameTime, unsigned long ServerFrameNr)
{
    if (IsThinking) return;

    IsThinking=true;

    // Beachte:
    // - Neu geschaffene Entities sollen nicht gleich 'Think()'en!
    //   Zur Erreichung vergleiche dazu die Implementation von EngineEntityT::Think().
    //   DÜRFTEN sie trotzdem gleich Think()en??? (JA!) Die OldStates kämen dann evtl. durcheinander!? (NEIN!)
    //   Allerdings übertragen wir mit BaseLines grundsätzlich KEINE Events (??? PRÜFEN!), Think()en macht insofern also nur eingeschränkt Sinn.
    // - EntityIDs sollten wohl besser NICHT wiederverwendet werden, da z.B. Parents die IDs ihrer Children speichern usw.
    // - Letzteres führt aber zu zunehmend vielen NULL-Pointern im EngineEntities-Array.
    // - Dies könnte sich evtl. mit einem weiteren Array von 'active EntityIDs' lösen lassen.
    for (unsigned long EntityNr=0; EntityNr<EngineEntities.Size(); EntityNr++)
        if (EngineEntities[EntityNr]!=NULL)
            EngineEntities[EntityNr]->PreThink(ServerFrameNr);

    // Must never move this above the PreThink() calls above, because the Game assumes that the entity states may
    // be modified (e.g. by map script commands) as soon as it gets this call.
    cf::GameSys::Game->Sv_BeginThinking(FrameTime);

    for (unsigned long EntityNr=0; EntityNr<EngineEntities.Size(); EntityNr++)
        if (EngineEntities[EntityNr]!=NULL)
            EngineEntities[EntityNr]->Think(FrameTime, ServerFrameNr);

    cf::GameSys::Game->Sv_EndThinking();

    IsThinking=false;


    // If entities removed other entities (or even themselves!) while thinking, remove them now.
    for (unsigned long RemoveNr=0; RemoveNr<EntityRemoveList.Size(); RemoveNr++)
    {
        const unsigned long EntityID=EntityRemoveList[RemoveNr];

        delete EngineEntities[EntityID];
        EngineEntities[EntityID]=NULL;
    }

    EntityRemoveList.Overwrite();
}


void EntityManagerT::WriteNewBaseLines(unsigned long SentClientBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const
{
    for (unsigned long EntityNr=0; EntityNr<EngineEntities.Size(); EntityNr++)
        if (EngineEntities[EntityNr]!=NULL)
            EngineEntities[EntityNr]->WriteNewBaseLine(SentClientBaseLineFrameNr, OutDatas);
}


void EntityManagerT::WriteFrameUpdateMessages(unsigned long ClientEntityID, unsigned long ServerFrameNr, unsigned long ClientFrameNr,
                                              ArrayT< ArrayT<unsigned long> >& ClientOldStatesPVSEntityIDs,
                                              unsigned long& ClientCurrentStateIndex, NetDataT& OutData) const
{
    // Wenn dies hier aufgerufen wird, befinden sich sämtliche EngineEntities schon im Zustand ('Entity->State') zum Frame 'ServerFrameNr'.
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
    unsigned long          ClientLeafNr        =(EngineEntities[ClientEntityID]!=NULL) ? Ca3DEWorld.GetWorld().BspTree->WhatLeaf(EngineEntities[ClientEntityID]->GetBaseEntity()->GetOrigin()) : 0;

    // Finde heraus, welche Entities im PVS von diesem Client liegen. Erhalte ein Array von EntityIDs.
    for (unsigned long EntityNr=0; EntityNr<EngineEntities.Size(); EntityNr++)
        if (EngineEntities[EntityNr]!=NULL)
        {
            const Vector3dT&      EntityOrigin=EngineEntities[EntityNr]->GetBaseEntity()->GetOrigin();
            BoundingBox3T<double> EntityBB    =EngineEntities[EntityNr]->GetBaseEntity()->GetDimensions();

            EntityBB.Min += EntityOrigin;
            EntityBB.Max += EntityOrigin;

            if (Ca3DEWorld.GetWorld().BspTree->IsInPVS(EntityBB, ClientLeafNr)) NewStatePVSEntityIDs->PushBack(EntityNr);
        }


    unsigned long DeltaFrameNr;     // Kann dies entfernen, indem der Packet-Header direkt im if-else-Teil geschrieben wird!

    if (ClientFrameNr==0 || ClientFrameNr>=ServerFrameNr || ClientFrameNr+ClientOldStatesPVSEntityIDs.Size()-1<ServerFrameNr)
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
        unsigned long FrameDiff=ServerFrameNr-ClientFrameNr;

        DeltaFrameNr        =ClientFrameNr;
        OldStatePVSEntityIDs=&ClientOldStatesPVSEntityIDs[FrameDiff<=ClientCurrentStateIndex ? ClientCurrentStateIndex-FrameDiff : ClientOldStatesPVSEntityIDs.Size()+ClientCurrentStateIndex-FrameDiff];
    }


    OutData.WriteByte(SC1_FrameInfo);
    OutData.WriteLong(ServerFrameNr);       // What we are delta'ing to   (Frame, für das wir Informationen schicken)
    OutData.WriteLong(DeltaFrameNr );       // What we are delta'ing from (Frame, auf das wir uns beziehen (0 für BaseLine))


    unsigned long OldIndex=0;
    unsigned long NewIndex=0;

    while (OldIndex<OldStatePVSEntityIDs->Size() || NewIndex<NewStatePVSEntityIDs->Size())
    {
        unsigned long OldEntityID=OldIndex<OldStatePVSEntityIDs->Size() ? (*OldStatePVSEntityIDs)[OldIndex] : 0x99999999;
        unsigned long NewEntityID=NewIndex<NewStatePVSEntityIDs->Size() ? (*NewStatePVSEntityIDs)[NewIndex] : 0x99999999;

        // Consider the following situation:
        // There are (or were) A REALLY BIG NUMBER of entities in the PVS of the current client entity ('EngineEntities[ClientEntityID]').
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
                if (!EngineEntities[NewEntityID]->WriteDeltaEntity(false /* send from baseline? */, ClientFrameNr, OutData, false))
                    EnqueueString("SERVER ERROR: %s, L %u: NewEntityID %u, ServerFrameNr %u, ClientFrameNr %u\n", __FILE__, __LINE__, NewEntityID, ServerFrameNr, ClientFrameNr);

            OldIndex++;
            NewIndex++;
            continue;
        }

        if (OldEntityID>NewEntityID)
        {
            // Dies ist ein neuer Entity, sende ihn von der BaseLine aus.
            // Deswegen kann der folgende Aufruf (gemäß der Spezifikation von WriteDeltaEntity()) auch nicht scheitern!
            if (!SkipEntity)
                EngineEntities[NewEntityID]->WriteDeltaEntity(true /* send from baseline? */, 0, OutData, true);

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


#if !DEDICATED

/*******************/
/*** Client Side ***/
/*******************/


bool EntityManagerT::CreateNewEntityFromEntityBaseLineMessage(NetDataT& InData)
{
    unsigned long EntityID    =InData.ReadLong();
    unsigned long EntityTypeID=InData.ReadLong();
    unsigned long EntityWFI   =InData.ReadLong();   // Short for: EntityWorldFileIndex

    const std::map<std::string, std::string>  EmptyMap;
    const unsigned long                       MFIndex =EntityWFI<Ca3DEWorld.GetWorld().GameEntities.Size() ? Ca3DEWorld.GetWorld().GameEntities[EntityWFI]->MFIndex : 0xFFFFFFFF;
    const std::map<std::string, std::string>& Props   =EntityWFI<Ca3DEWorld.GetWorld().GameEntities.Size() ? Ca3DEWorld.GetWorld().GameEntities[EntityWFI]->Properties : EmptyMap;
    const cf::SceneGraph::GenericNodeT*       RootNode=EntityWFI<Ca3DEWorld.GetWorld().GameEntities.Size() ? Ca3DEWorld.GetWorld().GameEntities[EntityWFI]->BspTree : NULL;
    const cf::ClipSys::CollisionModelT*       CollMdl =EntityWFI<Ca3DEWorld.GetWorld().GameEntities.Size() ? Ca3DEWorld.GetWorld().GameEntities[EntityWFI]->CollModel : NULL;

    // Register CollMdl also with the cf::ClipSys::CollModelMan, so that both the owner (Ca3DEWorld.GameEntities[EntityWFI])
    // as well as the game code can free/delete it in their destructors (one by "delete", the other by cf::ClipSys::CollModelMan->FreeCM()).
    cf::ClipSys::CollModelMan->GetCM(CollMdl);

    // Es ist nicht sinnvoll, CreateBaseEntityFromTypeID() in Parametern die geparsten InData-Inhalte zu übergeben (Origin, Velocity, ...),
    // denn spätestens bei der SequenceNr und FrameNr kommt es zu Problemen. Deshalb lieber erstmal ein BaseEntitiy mit "falschem" State erzeugen.
    BaseEntityT* NewBaseEntity=cf::GameSys::Game->CreateBaseEntityFromTypeNr(EntityTypeID, Props, RootNode, CollMdl, EntityID, EntityWFI, MFIndex, &Ca3DEWorld);

    // Dies kann nur passieren, wenn EntityTypeID ein unbekannter Typ ist! Ein solcher Fehler ist also fatal.
    // Andererseits sollte ein Disconnect dennoch nicht notwendig sein, der Fehler sollte ohnehin niemals auftreten.
    if (!NewBaseEntity)
    {
        // Finish reading InData, so that we can gracefully continue despite the error.
        InData.ReadDMsg();
        EnqueueString("CLIENT ERROR: %s, L %u: Cannot create entity %u from SC1_EntityBaseLine msg: unknown type ID '%u' (WorldFileIndex %lu, MapFileIndex %lu)!\n", __FILE__, __LINE__, EntityID, EntityTypeID, EntityWFI, MFIndex);
        return false;
    }

    // Falls notwendig, Platz für die neue EntityID schaffen.
    while (EngineEntities.Size()<=EntityID) EngineEntities.PushBack(NULL);

    // Die EntityID könnte durchaus wiederverwendet werden - was immer der Server wünscht.
    delete EngineEntities[EntityID];

    // Neuen Entity tatsächlich erschaffen.
    EngineEntities[EntityID]=new EngineEntityT(NewBaseEntity, InData);
    return true;
}


bool EntityManagerT::ParseServerDeltaUpdateMessage(unsigned long EntityID, unsigned long DeltaFrameNr, unsigned long ServerFrameNr, const ArrayT<uint8_t>* DeltaMessage)
{
    bool EntityIDIsOK=false;

    if (EntityID<EngineEntities.Size())
        if (EngineEntities[EntityID]!=NULL)
            EntityIDIsOK=true;

    if (!EntityIDIsOK)
    {
        // Gib Warnung aus. Aber nur, weil wir mit einer SC1_EntityUpdate Message nichts anfangen können, brauchen wir noch lange nicht zu disconnecten.
        // ONE reason for getting EntityID>=EngineEntities.Size() here is the way how baselines are sent:
        // When a client joins a level, there can be a LOT of entities. Usually, not all baselines of all entities fit into a single
        // realiable message at once, and thus the server sends them in batches, contained in subsequent realiable messages.
        // Between realiable messages however, the server sends also SC1_EntityUpdate messages.
        // These messages can already refer to entities that the client knows nothing about, because it has not yet seen the (reliable)
        // introductory baseline message, and so we get here.
        // I turn the "WARNING" into an "INFO", so that ordinary users get a better impression. ;)
        if (EntityID>=EngineEntities.Size()) EnqueueString("CLIENT INFO: %s, L %u: EntityID>=EngineEntities.Size()\n", __FILE__, __LINE__);
                                        else EnqueueString("CLIENT WARNING: %s, L %u: EngineEntities[EntityID]==NULL \n", __FILE__, __LINE__);
        EnqueueString("(EntityID==%u, EngineEntities.Size()==%u)\n", EntityID, EngineEntities.Size());
        return false;
    }

    // Gibt bei Scheitern Diagnose-Nachricht aus. Häufigster Grund für Scheitern dürfe eine zu alte DeltaFrameNr sein.
    // Der Calling-Code muß das erkennen und reagieren (durch Anfordern von nichtkomprimierten (gegen die BaseLine komprimierten) Messages).
    // Jedenfalls nicht Grund genug für ein Client-Disconnect.
    return EngineEntities[EntityID]->ParseServerDeltaUpdateMessage(DeltaFrameNr, ServerFrameNr, DeltaMessage);
}


bool EntityManagerT::Repredict(unsigned long OurEntityID, unsigned long RemoteLastIncomingSequenceNr, unsigned long LastOutgoingSequenceNr)
{
    if (OurEntityID<EngineEntities.Size())
        if (EngineEntities[OurEntityID]!=NULL)
            return EngineEntities[OurEntityID]->Repredict(RemoteLastIncomingSequenceNr, LastOutgoingSequenceNr);

    return false;
}


void EntityManagerT::Predict(unsigned long OurEntityID, const PlayerCommandT& PlayerCommand, unsigned long OutgoingSequenceNr)
{
    if (OurEntityID<EngineEntities.Size())
        if (EngineEntities[OurEntityID]!=NULL)
            EngineEntities[OurEntityID]->Predict(PlayerCommand, OutgoingSequenceNr);
}


bool EntityManagerT::GetCamera(unsigned long EntityID, bool UsePredictedState, Vector3dT& Origin, unsigned short& Heading, unsigned short& Pitch, unsigned short& Bank) const
{
    if (EntityID<EngineEntities.Size())
        if (EngineEntities[EntityID]!=NULL)
        {
            EngineEntities[EntityID]->GetCamera(UsePredictedState, Origin, Heading, Pitch, Bank);
            return true;
        }

    return false;
}


bool EntityManagerT::GetLightSourceInfo(unsigned long EntityID, unsigned long OurEntityID, unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const
{
    if (EntityID<EngineEntities.Size())
        if (EngineEntities[EntityID]!=NULL)
            return EngineEntities[EntityID]->GetLightSourceInfo(EntityID==OurEntityID, DiffuseColor, SpecularColor, Position, Radius, CastsShadows);

    return false;
}


void EntityManagerT::DrawEntities(unsigned long OurEntityID, bool SkipOurEntity, const VectorT& ViewerPos, const ArrayT<unsigned long>& EntityIDs) const
{
    for (unsigned long IDNr=0; IDNr<EntityIDs.Size(); IDNr++)
    {
        const unsigned long EntityID=EntityIDs[IDNr];

        if (EntityID<EngineEntities.Size())
            if (EngineEntities[EntityID]!=NULL)
            {
                const bool FirstPersonView  =(EntityID==OurEntityID);
                const bool UsePredictedState=(EntityID==OurEntityID);   // TODO: For correctness, we should refer to a global 'UsePrediction' variable here, instead of using always 'true' for "our entity" ('OurEntityID')!

                if (EntityID==OurEntityID && SkipOurEntity) continue;

                EngineEntities[EntityID]->Draw(FirstPersonView, UsePredictedState, ViewerPos);
            }
    }
}


void EntityManagerT::PostDrawEntities(float FrameTime, unsigned long OurEntityID, const ArrayT<unsigned long>& EntityIDs)
{
    for (unsigned long IDNr=0; IDNr<EntityIDs.Size(); IDNr++)
    {
        const unsigned long EntityID=EntityIDs[IDNr];

        if (EntityID!=OurEntityID && EntityID<EngineEntities.Size())
            if (EngineEntities[EntityID]!=NULL)
                EngineEntities[EntityID]->PostDraw(FrameTime, false, false);
    }

    if (OurEntityID<EngineEntities.Size())
        if (EngineEntities[OurEntityID]!=NULL)
            EngineEntities[OurEntityID]->PostDraw(FrameTime, true, true);
}

#endif   /* !DEDICATED */
