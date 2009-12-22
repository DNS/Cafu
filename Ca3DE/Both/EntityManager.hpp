/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

/*******************************/
/*** Entity Manager (Header) ***/
/*******************************/

#ifndef _ENTITYMANAGER_HPP_
#define _ENTITYMANAGER_HPP_

#ifdef _WIN32
    #if defined(_MSC_VER)
        #if (_MSC_VER<1300)
            #define for if (false) ; else for
        #endif
    #endif
#endif

#include "../../Games/BaseEntity.hpp"
#include <map>


class  Ca3DEWorldT;
struct PlayerCommandT;
class  EngineEntityT;
class  NetDataT;

namespace cf { namespace ClipSys { class CollisionModelT; } }
namespace cf { namespace SceneGraph { class GenericNodeT; } }


class EntityManagerT
{
    private:

    EntityManagerT(const EntityManagerT&);      // Use of the Copy    Constructor is not allowed
    void operator = (const EntityManagerT&);    // Use of the Assignment Operator is not allowed

    Ca3DEWorldT&           Ca3DEWorld;          // Wegen dieser Zeile wird eine Ca3DEWorld unverzichtbar bleiben!
    ArrayT<EngineEntityT*> EngineEntities;
    bool                   IsThinking;          // Set to true while we're thinking, so that our methods can detect recursive calls.
    ArrayT<unsigned long>  EntityRemoveList;    // List of entity IDs that were scheduled for removal while thinking.


    public:

    /******************/
    /*** Both Sides ***/
    /******************/

    EntityManagerT(Ca3DEWorldT& Ca3DEWorld_);
   ~EntityManagerT();

    // Füllt das Array 'EntityIDs' mit allen zur Zeit gültigen Entity-IDs.
    // Hauptsächlich für Aufrufe aus dem EntityServiceInterface gedacht.
    void GetAllEntityIDs(ArrayT<unsigned long>& EntityIDs) const;

    // Gibt einen Zeiger auf den BaseEntity mit der ID 'EntityID' zurück, und NULL, wenn ein Entity mit dieser ID nicht existiert.
    // Hauptsächlich für Aufrufe aus dem EntityServiceInterface gedacht.
    // Nützlich aber auch für die Engine, wenn sie Details über einen Entity wissen möchte (z.B. bei Lichtquellen).
    // Der zurückgegebene Zeiger kann leider nicht 'const' sein, weil damit ansonsten z.B. BaseEntityT::ProcessConfigString() nicht aufgerufen werden kann.
    BaseEntityT* GetBaseEntityByID(unsigned long EntityID) const;

    // Läßt den EngineEntity mit der ID 'EntityID' den 'ConfigString' und die 'ConfigData' bearbeiten.
    void ProcessConfigString(unsigned long EntityID, const void* ConfigData, const char* ConfigString);


    /*******************/
    /*** Server Side ***/
    /*******************/

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
    void RemoveEntity(unsigned long EntityID);

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


    /*******************/
    /*** Client Side ***/
    /*******************/

    // Erzeugt einen neuen Entity durch das zu Ende Lesen einer SC1_EntityBaseLine Message. Gibt 'true' zurück bei Erfolg, sonst 'false'.
    // Letzteres passiert nur nach einem fatalen Fehler, nämlich wenn 'InData' einen unbekannten Entity-Typ beschreibt (Entity-TypeID).
    bool CreateNewEntityFromEntityBaseLineMessage(NetDataT& InData);

    // Ruft 'EngineEntityT::ParseServerDeltaUpdateMessage()' für den EngineEntityT mit der ID 'EntityID' auf (siehe Dokumentation dieser Funktion!).
    // Das Rückgabeergebnis entspricht dem dieser Funktion, ein Scheitern kann nun aber zusätzlich vorkommen, falls 'EntityID' nicht existiert.
    // 'InData' wird auf jeden Fall gemäß der 'FieldMask' weitergelesen (selbst wenn der Entity mit ID 'EntityID' nicht existiert),
    // sodaß es zum Auslesen weiterer Messages verwandt werden kann.
    // D.h. insbesondere, daß der Aufruf dieser Funktion nach Gelingen oder Scheitern NICHT wiederholt werden kann!
    bool ParseServerDeltaUpdateMessage(unsigned long EntityID, unsigned long DeltaFrameNr, unsigned long ServerFrameNr,
                                       unsigned long FieldMask, NetDataT& InData);

    // Please see the corresponding function in EngineEntityT for documentation.
    bool Repredict(unsigned long OurEntityID, unsigned long RemoteLastIncomingSequenceNr, unsigned long LastOutgoingSequenceNr);

    // Please see the corresponding function in EngineEntityT for documentation.
    void Predict(unsigned long OurEntityID, const PlayerCommandT& PlayerCommand, unsigned long OutgoingSequenceNr);

    // Please see the corresponding function in EngineEntityT for documentation.
    const EntityStateT* GetPredictedState(unsigned long OurEntityID);

    // Please see the corresponding function in EngineEntityT for documentation.
    bool GetLightSourceInfo(unsigned long EntityID, unsigned long OurEntityID, unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const;

    // Draws all entities whose ID is contained in the 'EntityIDs' array.
    // The entity with ID 'OurEntityID' specifies "our" entity.
    // (Everything else is drawn from the viewpoint from this entity, and it is necessary to let the
    //  render code know that, such that (for example) it can prevent that we see the inside of our own body!)
    // Note that the Material Systems global per-lightsource lighting parameters (light-source and eye position etc.)
    // should have been set before calling this function. Ambient light color is however set within this function (per entity).
    void DrawEntities(unsigned long OurEntityID, bool SkipOurEntity, const VectorT& ViewerPos, const ArrayT<unsigned long>& EntityIDs) const;

    // Calls the 'PostDraw()' methods of all entities whose ID is contained in the 'EntityIDs' array.
    // The calls are ordered such that the call to the entity with ID 'OurEntityID' is made last.
    // The 'FrameTime' is passed to each call of 'PostDraw()'.
    // All this provides opportunities for entities to render HUDs, employ simple "mini-prediction",
    // triggers sounds, register particles, do other server-independent eye-candy, and so on.
    void PostDrawEntities(float FrameTime, unsigned long OurEntityID, const ArrayT<unsigned long>& EntityIDs);
};

#endif
