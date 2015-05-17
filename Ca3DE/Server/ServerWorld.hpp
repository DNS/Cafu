/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_CASERVERWORLD_HPP_INCLUDED
#define CAFU_CASERVERWORLD_HPP_INCLUDED

#include "../Ca3DEWorld.hpp"
#include "../../Games/PlayerCommand.hpp"


namespace cf { namespace ClipSys { class CollisionModelT; } }
struct ClientInfoT;
class NetDataT;


class CaServerWorldT : public Ca3DEWorldT
{
    public:

    // Erstellt eine neue ServerWorld anhand des World-Files 'FileName', wobei 'FileName' den kompletten (wenn auch relativen) Pfad und Namen enthält.
    CaServerWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes);

    /// Removes the entity identified by 'EntityID' from the (server) world.
    void RemoveEntity(unsigned long EntityID);

    // Fügt einen neuen HumanPlayer-Entity zum NÄCHSTEN Frame in die World ein (idR nach Client-Join oder World-Change),
    // NICHT ins aktuelle (bzgl. der BaseLineFrameNr). Ziel: Erreiche gleiches Verhalten wie z.B. das des MonsterMakers.
    // Gibt bei Erfolg die ID des neuen Entities zurück, sonst 0xFFFFFFFF.
    unsigned long InsertHumanPlayerEntityForNextFrame(const char* PlayerName, const char* ModelName, unsigned long ClientInfoNr);

    // Informiert den (HumanPlayer-)Entity mit der ID 'HumanPlayerEntityID' über das 'PlayerCommand' (zur Verarbeitung beim nächsten 'Think()en'.
    void NotifyHumanPlayerEntityOfClientCommand(unsigned long HumanPlayerEntityID, const PlayerCommandT& PlayerCommand);

    // Falls es neue Entities (und damit neue BaseLine-Messages) gibt, die jünger sind als 'OldBaseLineFrameNr',
    // schreibe entsprechende BaseLine-Messages nach 'OutDatas'.
    // Schreibt für alle EngineEntities, die seit 'SentClientBaseLineFrameNr' (=='OldBaseLineFrameNr') neu erschaffen wurden, SC1_EntityBaseLine Messages nach 'OutDatas'
    // (d.h. für solche, deren 'BaseLineFrameNr' größer (d.h. jünger) als die 'SentClientBaseLineFrameNr' ist).
    unsigned long WriteClientNewBaseLines(unsigned long OldBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const;

    /// This method writes a delta update message for the given client into the given `OutData`.
    /// Note that this method must only be called *after* UpdateFrameInfo() has been called,
    /// because it relies on the client's frame info being up-to-date for the current frame!
    ///
    /// The message consist of an SC1_FrameInfo header and all SC1_EntityUpdate and
    /// SC1_EntityRemove (sub-)messages as required for the client to reconstruct the current
    /// frame.
    void WriteClientDeltaUpdateMessages(const ClientInfoT& ClientInfo, NetDataT& OutData) const;

    /// This method advances the world over the given time `FrameTime` into the next state.
    /// That is, time `FrameTime` is applied to all entities in `m_EngineEntities` in order
    /// to compute the next state, advancing the `m_ServerFrameNr` by one.
    void Think(float FrameTime);

    /// This method *must* be called after Think() for each client:
    /// It updates the client's frame info corresponding to the the new/current server frame.
    ///
    /// The previous call to Think() brought the entire world (all m_EngineEntities) into the
    /// new state with number m_ServerFrameNr. The ClientInfoT instances however know at this
    /// time nothing about the new state. Calling this method updates a client's frame info
    /// (the list of entities that are relevant for the client in the new frame) accordingly.
    void UpdateFrameInfo(ClientInfoT& ClientInfo) const;


    private:

    CaServerWorldT(const CaServerWorldT&);      ///< Use of the Copy Constructor    is not allowed.
    void operator = (const CaServerWorldT&);    ///< Use of the Assignment Operator is not allowed.

    unsigned long         m_ServerFrameNr;      ///< Nummer des aktuellen Frames/Zustands
    bool                  m_IsThinking;         ///< Set to true while we're thinking, so that our methods can detect recursive calls.
    ArrayT<unsigned long> m_EntityRemoveList;   ///< List of entity IDs that were scheduled for removal while thinking.
};

#endif
