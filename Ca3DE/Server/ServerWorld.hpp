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

#ifndef CAFU_CASERVERWORLD_HPP_INCLUDED
#define CAFU_CASERVERWORLD_HPP_INCLUDED

#include "../Both/Ca3DEWorld.hpp"
#include "../../Games/PlayerCommand.hpp"


namespace cf { namespace ClipSys { class CollisionModelT; } }
class NetDataT;


class CaServerWorldT : public Ca3DEWorldT
{
    public:

    // Erstellt eine neue ServerWorld anhand des World-Files 'FileName', wobei 'FileName' den kompletten (wenn auch relativen) Pfad und Namen enthält.
    CaServerWorldT(const char* FileName, ModelManagerT& ModelMan);

    // Der Destruktor.
    ~CaServerWorldT();

    // The virtual methods inherited from the base class GameWorldI.
    unsigned long CreateNewEntity(const std::map<std::string, std::string>& Properties, unsigned long CreationFrameNr, const VectorT& Origin);
    void          RemoveEntity(unsigned long EntityID);

    // Fügt einen neuen HumanPlayer-Entity zum NÄCHSTEN Frame in die World ein (idR nach Client-Join oder World-Change),
    // NICHT ins aktuelle (bzgl. der BaseLineFrameNr). Ziel: Erreiche gleiches Verhalten wie z.B. das des MonsterMakers.
    // Gibt bei Erfolg die ID des neuen Entities zurück, sonst 0xFFFFFFFF.
    unsigned long InsertHumanPlayerEntityForNextFrame(const char* PlayerName, const char* ModelName, unsigned long ClientInfoNr);

    // Entfernt den (HumanPlayer-)Entity mit der ID 'HumanPlayerEntityID' aus der World.
    void RemoveHumanPlayerEntity(unsigned long HumanPlayerEntityID);

    // Informiert den (HumanPlayer-)Entity mit der ID 'HumanPlayerEntityID' über das 'PlayerCommand' (zur Verarbeitung beim nächsten 'Think()en'.
    void NotifyHumanPlayerEntityOfClientCommand(unsigned long HumanPlayerEntityID, const PlayerCommandT& PlayerCommand);

    // Falls es neue Entities (und damit neue BaseLine-Messages) gibt, die jünger sind als 'OldBaseLineFrameNr',
    // schreibe entsprechende BaseLine-Messages nach 'OutDatas'.
    unsigned long WriteClientNewBaseLines(unsigned long OldBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const;

    // Schreibt eine komplette Delta-Update-Message (FrameInfo+EntityUpdates) nach 'OutData'.
    // 'ClientEntityID' ist dabei die ID des Entities, dessen PVS zur Bestimmung der sichtbaren Entities herangezogen wird,
    // 'ClientFrameNr' die Nummer des Frames/Zustands, den der Client zuletzt bestätigt hat,
    // 'ClientOldStatesPVSEntityIDs' die von dieser Funktion gewarteteten PVS-Informationen vorangegangener Zustände und
    // 'ClientCurrentStateIndex' der (ebenfalls von dieser Funktion gewartete) Index in die PVS-Informationen.
    void WriteClientDeltaUpdateMessages(unsigned long ClientEntityID, unsigned long ClientFrameNr, ArrayT< ArrayT<unsigned long> >& ClientOldStatesPVSEntityIDs, unsigned long& ClientCurrentStateIndex, NetDataT& OutData) const;

    // Überführt die World über die Zeit 'FrameTime' in den nächsten Zustand.
    void Think(float FrameTime);


    private:

    CaServerWorldT(const CaServerWorldT&);      ///< Use of the Copy Constructor    is not allowed.
    void operator = (const CaServerWorldT&);    ///< Use of the Assignment Operator is not allowed.

    // Erzeugt einen neuen Entity. IdR wird dies [1] nach dem Laden einer neuen World aufgerufen werden, mit den im World-File enthaltenen Parametern.
    // Denkbar ist aber auch [2] ein Aufruf aus dem 'EngineServiceInterface', wenn ein Entity einen neuen Entity erzeugen möchte.
    // Dritte Möglichkeit: [3] Aufruf aus ServerWorldT::InsertHumanPlayerEntity(), zum Anlegen von HumanPlayer-Entities für Clients.
    // (Dies erfolgt aus ClientInfoT::InitForNewWorld(): Entweder wenn ein Client neu dazukommt, oder nach einem World-change für die vorhandenen Clients.)
    // 'EntityMapFileID' ist ein Handle zu den Map-File-Informationen des Entities, falls vorhanden (d.h. Fall [1]), sonst (Fälle [2] und [3]) -1.
    // 'CreationFrameNr' ist die Nummer des Frames, zu dem der Entity erzeugt wird, sollte also stets die 'ServerFrameNr' der ServerWorld sein.
    // Der Rückgabewert ist die ID des erzeugten Entities. (Verwendung: Der Server kann den Client damit wissen lassen, welcher Entity der Client ist).
    // Falls der Entity nicht erzeugt werden kann, wird 0xFFFFFFFF zurückgegeben.
    unsigned long CreateNewEntityFromBasicInfo(const std::map<std::string, std::string>& Properties, const cf::SceneGraph::GenericNodeT* RootNode,
        const cf::ClipSys::CollisionModelT* CollisionModel, unsigned long WorldFileIndex, unsigned long MapFileIndex, unsigned long CreationFrameNr, const VectorT& Origin,
        const char* PlayerName=NULL, const char* ModelName=NULL);

    // Entfernt einen EngineEntity. Beispiele für Aufruf-Möglichkeiten:
    // a) aus ServerWorldT::RemoveHumanPlayerEntity(), wenn ein HumanPlayer-Entity eines Clients gelöscht werden soll.
    // b) aus dem 'EntityServiceInterface', wenn ein Entity z.B. seine Children oder sich selbst löschen will.
    // Die Clients bekommen unabhängig davon in einer SC1_DropClient Message explizit mitgeteilt, wenn ein Client (warum auch immer) den Server verläßt.
    // Den dazugehörigen Entity muß der Client deswegen aber nicht unbedingt sofort und komplett aus seiner World entfernen,
    // dies sollte vielmehr durch Wiederverwendung von EntityIDs durch den Server geschehen!
    void RemoveEntity_(unsigned long EntityID);

    // Berechnet den nächsten Zustand 'ServerFrameNr' der EngineEntities, indem auf alle Entities die 'FrameTime' angewandt wird.
    void Think(float FrameTime, unsigned long ServerFrameNr);

    // Schreibt für alle EngineEntities, die seit 'SentClientBaseLineFrameNr' neu erschaffen wurden, SC1_EntityBaseLine Messages nach 'OutDatas'
    // (d.h. für solche, deren 'BaseLineFrameNr' größer (d.h. jünger) als die 'SentClientBaseLineFrameNr' ist).
    void WriteNewBaseLines(unsigned long SentClientBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const;

    // Diese Funktion nimmt folgende Parameter zu einem Client entgegen:
    // 'ClientEntityID'              - die ID des Entities des Clients,
    // 'ClientFrameNr'               - die Nummer des letzten ServerFrames, das der Client von uns gesehen hat,
    // 'ClientOldStatesPVSEntityIDs' - die IDs der Entities der letzten Zustände des Clients, und
    // 'ClientCurrentStateIndex'     - der zum Zustand des Frames 'ClientFrameNr' gehörende Index ins 'ClientOldStatesPVSEntityIDs' Array.
    // Diese Funktion schreibt dann eine SC1_NewFrameInfo Message und die sich aus obigem ergebenden, relevanten SC1_EntityUpdate Messages nach 'OutData',
    // sodaß die Gegenstelle aus dem Zustand des Frames 'ClientFrameNr' den Zustand des Frames 'ServerFrameNr' rekonstruieren kann.
    // WICHTIG: Die EngineEntities befinden sich bei Funktionsaufruf schon im Zustand des Frames 'ServerFrameNr'. Die Client PVS-EntityIDs für diesen
    // Zustand werden erst mit diesem Aufruf erstellt! Deshalb MUSS diese Funktion auch nach JEDEM Aufruf von 'Think()' für jeden Client aufgerufen werden!
    void WriteFrameUpdateMessages(unsigned long ClientEntityID, unsigned long ServerFrameNr, unsigned long ClientFrameNr,
                                  ArrayT< ArrayT<unsigned long> >& ClientOldStatesPVSEntityIDs,
                                  unsigned long& ClientCurrentStateIndex, NetDataT& OutData) const;


    unsigned long         m_ServerFrameNr;      ///< Nummer des aktuellen Frames/Zustands
    bool                  m_IsThinking;         ///< Set to true while we're thinking, so that our methods can detect recursive calls.
    ArrayT<unsigned long> m_EntityRemoveList;   ///< List of entity IDs that were scheduled for removal while thinking.
};

#endif
