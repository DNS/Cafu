/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "EngineEntity.hpp"
#include "NetConst.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Network/Network.hpp"
#include "Win32/Win32PrintHelp.hpp"
#include "Ca3DEWorld.hpp"
#include "GameSys/CompHumanPlayer.hpp"
#include "GameSys/Entity.hpp"
#include "GameSys/Interpolator.hpp"


namespace
{
    ConVarT UsePrediction("usePrediction", true, ConVarT::FLAG_MAIN_EXE,
        "Toggles whether client prediction is used (recommended!).");

    ConVarT clientApproxNPCs("clientApproxNPCs", true, ConVarT::FLAG_MAIN_EXE,
        "Toggles whether origins and other values are interpolated over client "
        "frames in order to bridge the larger intervals between server frames.");
}


/******************/
/*** Both Sides ***/
/******************/


cf::Network::StateT EngineEntityT::GetState() const
{
    cf::Network::StateT     State;
    cf::Network::OutStreamT Stream(State);

    m_Entity->Serialize(Stream);

    return State;
}


void EngineEntityT::SetState(const cf::Network::StateT& State, bool IsIniting) const
{
    cf::Network::InStreamT Stream(State);

    m_Entity->Deserialize(Stream, IsIniting);

    // Deserialization has brought new reference values for interpolated values.
    for (unsigned int CompNr = 0; true; CompNr++)
    {
        IntrusivePtrT<cf::GameSys::ComponentBaseT> Comp = m_Entity->GetComponent(CompNr);

        if (Comp == NULL) break;

        for (unsigned int i = 0; i < Comp->GetInterpolators().Size(); i++)
        {
            if (IsIniting || !clientApproxNPCs.GetValueBool())
            {
                Comp->GetInterpolators()[i]->ReInit();
            }
            else
            {
                Comp->GetInterpolators()[i]->UpdateTargetValue();
            }
        }
    }
}


/*******************/
/*** Server Side ***/
/*******************/


EngineEntityT::EngineEntityT(IntrusivePtrT<cf::GameSys::EntityT> Ent, unsigned long CreationFrameNr)
    : m_Entity(Ent),
      EntityStateFrameNr(CreationFrameNr),
      m_BaseLine(),
      m_BaseLineFrameNr(CreationFrameNr),
      m_OldStates()
{
    m_BaseLine=GetState();

    for (unsigned long OldStateNr=0; OldStateNr<16 /*MUST be a power of 2*/; OldStateNr++)
        m_OldStates.PushBack(m_BaseLine);
}


void EngineEntityT::PreThink(unsigned long ServerFrameNr)
{
    // The goal of the Think() methods is to compute the state for frame ServerFrameNr.
    // Thus, here, before the thinking starts, record the state of the previous frame.
    //
    // Note that if this entity has only been created in ServerFrameNr, there is no previous state to record.
    // As our earliest state is `m_BaseLineFrameNr`, we must have `m_BaseLineFrameNr <= ServerFrameNr - 1`,
    // thus `m_BaseLineFrameNr < ServerFrameNr`. (If `m_BaseLineFrameNr == ServerFrameNr` was allowed, this
    // would imply that we had an old state at `m_OldStates[m_BaseLineFrameNr - 1]` and result in
    // `m_BaseLine != m_OldStates[m_BaseLineFrameNr]`.)
    if (m_BaseLineFrameNr >= ServerFrameNr) return;

    m_OldStates[(ServerFrameNr-1) & (m_OldStates.Size()-1)] = GetState();
}


void EngineEntityT::Think(float FrameTime, unsigned long ServerFrameNr)
{
    // As detailed in PreThink().
    if (m_BaseLineFrameNr >= ServerFrameNr) return;

    // This computes the state of the entity for frame ServerFrameNr.
    m_Entity->OnServerFrame(FrameTime);

    EntityStateFrameNr=ServerFrameNr;
}


void EngineEntityT::WriteNewBaseLine(unsigned long SentClientBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const
{
    // Nur dann etwas tun, wenn unsere 'BaseLineFrameNr' größer (d.h. jünger) als 'SentClientBaseLineFrameNr' ist,
    // d.h. unsere 'BaseLineFrameNr' noch nie / noch nicht an den Client gesendet wurde.
    if (SentClientBaseLineFrameNr >= m_BaseLineFrameNr) return;

    NetDataT NewBaseLineMsg;

    NewBaseLineMsg.WriteByte(SC1_EntityBaseLine);
    NewBaseLineMsg.WriteLong(m_Entity->GetID());
    NewBaseLineMsg.WriteLong(m_Entity->GetParent().IsNull() ? UINT_MAX : m_Entity->GetParent()->GetID());
    NewBaseLineMsg.WriteDMsg(m_BaseLine.GetDeltaMessage(cf::Network::StateT() /*::ALL_ZEROS*/));

    OutDatas.PushBack(NewBaseLineMsg.Data);
}


bool EngineEntityT::WriteDeltaEntity(bool SendFromBaseLine, unsigned long ClientFrameNr, NetDataT& OutData, bool ForceInfo) const
{
    // Prüfe, ob die Voraussetzungen für die Parameter (insb. 'ClientFrameNr') eingehalten werden.
    if (!SendFromBaseLine)
    {
        // EntityStateFrameNr wird in Think() gesetzt und ist gleich der ServerFrameNr!
        // Beachte: OldStates speichert die alten Zustände von ServerFrameNr-1 bis ServerFrameNr-16.
        const unsigned long FrameDiff = EntityStateFrameNr - ClientFrameNr;

        if (FrameDiff < 1 || FrameDiff > m_OldStates.Size()) return false;
    }


    const cf::Network::StateT CurrentState = GetState();
    const ArrayT<uint8_t>     DeltaMsg     = CurrentState.GetDeltaMessage(SendFromBaseLine ? m_BaseLine : m_OldStates[ClientFrameNr & (m_OldStates.Size()-1)]);

    if (cf::Network::StateT::IsDeltaMessageEmpty(DeltaMsg) && !ForceInfo) return true;

    // Write the SC1_EntityUpdate message
    OutData.WriteByte(SC1_EntityUpdate);
    OutData.WriteLong(m_Entity->GetID());
    OutData.WriteDMsg(DeltaMsg);

    return true;
}


#if !DEDICATED

/*******************/
/*** Client Side ***/
/*******************/


EngineEntityT::EngineEntityT(IntrusivePtrT<cf::GameSys::EntityT> Ent, NetDataT& InData)
    : m_Entity(Ent),
      EntityStateFrameNr(0),
      m_BaseLine(),
      m_BaseLineFrameNr(1234),    // The m_BaseLineFrameNr is unused on the client.
      m_OldStates()
{
    const cf::Network::StateT CurrentState(cf::Network::StateT() /*::ALL_ZEROS*/, InData.ReadDMsg());

    // Pass true for the IsInited parameter in order to indicate that we're constructing the entity.
    // This is done in order to have it not wrongly process the event counters.
    SetState(CurrentState, true);

    for (unsigned long OldStateNr=0; OldStateNr<32 /*MUST be a power of 2*/; OldStateNr++)
        m_OldStates.PushBack(CurrentState);

    m_BaseLine=CurrentState;
}


bool EngineEntityT::ParseServerDeltaUpdateMessage(unsigned long DeltaFrameNr, unsigned long ServerFrameNr, const ArrayT<uint8_t>* DeltaMessage)
{
    // Sanity-Check: Wir wollen, daß 'DeltaFrameNr<=EntityStateFrameNr<ServerFrameNr' gilt.
    // Wäre 'DeltaFrameNr>EntityStateFrameNr', so sollten wir gegen einen State dekomprimieren, der in der Zukunft liegt.
    // Wäre 'EntityStateFrameNr>=ServerFrameNr', so sollten wir uns in einen State begeben, der schon Vergangenheit ist.
    // Dies hält auch für den Spezialfall 'DeltaFrameNr==0' (Delta-Dekompression gegen die BaseLine).
    // Im Normalfall 'DeltaFrameNr>0' müssen wir unten außerdem noch sicherstellen, daß der DeltaState nicht zu weit in der Vergangenheit liegt.
    //
    // ONE possible reason for DeltaFrameNr>EntityStateFrameNr is related to the way how baselines are sent,
    // see EntityManager.cpp, EntityManagerT::ParseServerDeltaUpdateMessage() for a description, which is essentially repeated here:
    // When a client joins a level, there can be a LOT of entities. Usually, not all baselines of all entities fit into a single
    // realiable message at once, and thus the server sends them in batches, contained in subsequent realiable messages.
    // Between realiable messages however, the server sends also SC1_EntityUpdate messages.
    // These messages can already refer to entities that the client knows nothing about, because it has not yet seen the (reliable)
    // introductory baseline message.
    // Then, the entities that the client already knows about normally receive and process delta updates here in this function,
    // the others don't (because their non-presence is already detected in EntityManagerT::ParseServerDeltaUpdateMessage()).
    // However, the frame counters increase normally, as if all entities were present. When finally the remaining entities
    // arrive (because their baseline got finally through), these entities are likely to have DeltaFrameNr>EntityStateFrameNr.
    // I turn the "WARNING" into an "INFO", so that ordinary users get a better impression. ;)
    if (DeltaFrameNr>EntityStateFrameNr)   { EnqueueString("CLIENT INFO: %s, L %u: DeltaFrameNr>EntityStateFrameNr (%lu>%lu)\n"  , __FILE__, __LINE__, DeltaFrameNr, EntityStateFrameNr); return false; }
    if (EntityStateFrameNr>=ServerFrameNr) { EnqueueString("CLIENT WARNING: %s, L %u: EntityStateFrameNr>=ServerFrameNr (%lu>%lu)\n", __FILE__, __LINE__, EntityStateFrameNr, ServerFrameNr); return false; }


    // Determine the source state to delta-decompress against (an old state or the baseline).
    const cf::Network::StateT* DeltaState=NULL;

    if (DeltaFrameNr>0)
    {
        // Der oben angekündigte Test, ob der DeltaState nicht schon zu weit in der Vergangenheit liegt.
        // Einen gültigen State können wir dann nicht mehr produzieren, und dem Calling-Code muß klar sein oder klar werden,
        // daß er gegen die BaseLines komprimierte Messages anfordern muß.
        if (EntityStateFrameNr-DeltaFrameNr >= m_OldStates.Size())
        {
            EnqueueString("CLIENT WARNING: %s, L %u: Delta state too old!\n", __FILE__, __LINE__);
            return false;
        }

        DeltaState = &m_OldStates[DeltaFrameNr & (m_OldStates.Size()-1)];
    }
    else
    {
        DeltaState = &m_BaseLine;
    }

    // Set the result as the new entity state, and record it in the m_OldStates for future reference.
    EntityStateFrameNr = ServerFrameNr;

    const cf::Network::StateT NewState = DeltaMessage ? cf::Network::StateT(*DeltaState, *DeltaMessage) : *DeltaState;

    m_OldStates[EntityStateFrameNr & (m_OldStates.Size()-1)] = NewState;
    SetState(NewState, DeltaFrameNr == 0);  // Don't process events and don't interpolate if we're delta'ing from the baseline (e.g. when re-entering the PVS).
    return true;
}


void EngineEntityT::Predict(const PlayerCommandT& PrevPlayerCommand, const PlayerCommandT& PlayerCommand)
{
    if (!UsePrediction.GetValueBool())
        return;

    IntrusivePtrT<cf::GameSys::ComponentHumanPlayerT> CompHP =
        dynamic_pointer_cast<cf::GameSys::ComponentHumanPlayerT>(m_Entity->GetComponent("HumanPlayer"));

    if (CompHP == NULL)
    {
        EnqueueString("WARNING - Prediction impossible: HumanPlayer component not found in human player entity!\n");
        return;
    }

    // Note that components other than CompHP should *not* Think/Repredict,
    // e.g. the player's CollisionModel component must not cause OnTrigger() callbacks!
    CompHP->Think(PrevPlayerCommand, PlayerCommand, false /*ThinkingOnServerSide*/);
}

#endif   /* !DEDICATED */
