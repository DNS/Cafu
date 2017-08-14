/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CASERVERWORLD_HPP_INCLUDED
#define CAFU_CASERVERWORLD_HPP_INCLUDED

#include "../Ca3DEWorld.hpp"
#include "../PlayerCommand.hpp"


namespace cf { namespace ClipSys { class CollisionModelT; } }
struct ClientInfoT;
class NetDataT;


class CaServerWorldT : public Ca3DEWorldT
{
    public:

    // Erstellt eine neue ServerWorld anhand des World-Files 'FileName', wobei 'FileName' den kompletten (wenn auch relativen) Pfad und Namen enthält.
    CaServerWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes);

    /// This method advances the world over the given time `FrameTime` into the next state.
    /// That is, time `FrameTime` is applied to all entities in `m_EngineEntities` in order
    /// to compute the next state, advancing the `m_ServerFrameNr` by one.
    /// Additionally, the player commands in each client's `ClientInfoT` are applied to the
    /// client's entity.
    /// Human player entities that are no longer referred to by any client are removed.
    void Think(float FrameTime, const ArrayT<ClientInfoT*>& ClientInfos);

    /// Inserts a new human player entity into the current frame.
    /// This method must be called after Think() and before WriteClientNewBaseLines().
    /// Returns the ID of the newly created entity or 0 on failure.
    unsigned int InsertHumanPlayerEntity(const std::string& PlayerName, const std::string& ModelName, unsigned int ClientInfoNr);

    // Falls es neue Entities (und damit neue BaseLine-Messages) gibt, die jünger sind als 'OldBaseLineFrameNr',
    // schreibe entsprechende BaseLine-Messages nach 'OutDatas'.
    // Schreibt für alle EngineEntities, die seit 'SentClientBaseLineFrameNr' (=='OldBaseLineFrameNr') neu erschaffen wurden, SC1_EntityBaseLine Messages nach 'OutDatas'
    // (d.h. für solche, deren 'BaseLineFrameNr' größer (d.h. jünger) als die 'SentClientBaseLineFrameNr' ist).
    unsigned long WriteClientNewBaseLines(unsigned long OldBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const;

    /// This method *must* be called after Think() for each client:
    /// It updates the client's frame info corresponding to the the new/current server frame.
    ///
    /// The previous call to Think() brought the entire world (all m_EngineEntities) into the
    /// new state with number m_ServerFrameNr. The ClientInfoT instances however know at this
    /// time nothing about the new state. Calling this method updates a client's frame info
    /// (the list of entities that are relevant for the client in the new frame) accordingly.
    void UpdateFrameInfo(ClientInfoT& ClientInfo) const;

    /// This method writes a delta update message for the given client into the given `OutData`.
    /// Note that this method must only be called *after* UpdateFrameInfo() has been called,
    /// because it relies on the client's frame info being up-to-date for the current frame!
    ///
    /// The message consist of an SC1_FrameInfo header and all SC1_EntityUpdate and
    /// SC1_EntityRemove (sub-)messages as required for the client to reconstruct the current
    /// frame.
    void WriteClientDeltaUpdateMessages(const ClientInfoT& ClientInfo, NetDataT& OutData) const;


    private:

    CaServerWorldT(const CaServerWorldT&);      ///< Use of the Copy Constructor    is not allowed.
    void operator = (const CaServerWorldT&);    ///< Use of the Assignment Operator is not allowed.

    unsigned long m_ServerFrameNr;  ///< Nummer des aktuellen Frames/Zustands
};

#endif
