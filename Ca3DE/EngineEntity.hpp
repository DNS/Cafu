/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_ENGINE_ENTITY_HPP_INCLUDED
#define CAFU_ENGINE_ENTITY_HPP_INCLUDED

#include "PlayerCommand.hpp"
#include "Math3D/Vector3.hpp"
#include "Network/State.hpp"
#include "Templates/Pointer.hpp"


class NetDataT;
namespace cf { namespace GameSys { class EntityT; } }


class EngineEntityT
{
    public:

    /******************/
    /*** Both Sides ***/
    /******************/

    /// Returns the GameSys entity related to this EngineEntityT.
    IntrusivePtrT<cf::GameSys::EntityT> GetEntity() const { return m_Entity; }


    /*******************/
    /*** Server Side ***/
    /*******************/

    // Creates a new EngineEntityT, where 'Entity_' points to a formerly constructed GameEntityI
    // and 'CreationFrameNr' is the number of the server frame for which this EngineEntityT is created.
    // Server side EngineEntityT creation is always (and only) triggered by an EntityManagerT, either after a new world was loaded or
    // when a game entity creates a new entity by calling GameWorld->CreateNewEntity() in its 'Think()' method.
    EngineEntityT(IntrusivePtrT<cf::GameSys::EntityT> Ent, unsigned long CreationFrameNr);

    // Prepares the entity to enter the next state for frame 'ServerFrameNr'.
    // This function must be called for each entity before any entities 'Think()' function is called.
    // Reason: Any thinking entity may also (implicitly) modify *other* entities, e.g. by calling their 'TakeDamage()' functions.
    // If such entities had not been previously prepared by this function, the implementation could not keep the state management right.
    void PreThink(unsigned long ServerFrameNr);

    // Computes the new state for the next frame 'ServerFrameNr', where 'FrameTime' seconds passed since the previous frame.
    // Does nothing if the Entity was just new created for the frame 'ServerFrameNr', that is, CreationFrameNr>=ServerFrameNr.
    // Do not use this method for client side prediction -- it will not work (before calling this method, calls to 'PreThink()'
    // are required (as detailed above), which in turn maintain 'OldStates').
    void Think(float FrameTime, unsigned long ServerFrameNr);

    // Schreibt die SC1_EntityBaseLine Message für diesen EngineEntityT nach 'OutDatas', falls 'SentClientBaseLineFrameNr' kleiner (d.h. älter)
    // als die 'BaseLineFrameNr' dieses EngineEntityTs ist, d.h. falls dieser EngineEntityT neu erschaffen wurde und an den Client noch keine
    // entsprechende SC1_EntityBaseLine Message geschickt wurde.
    // 'SentClientBaseLineFrameNr' ist die FrameNr, bis zu der dem Client schon alle SC1_EntityBaseLine Messages gesandt wurden
    // (heißt aber nicht, daß er die auch schon hat!). Unabhängig davon sollten die 'OutDatas' "reliable" verschickt werden.
    void WriteNewBaseLine(unsigned long SentClientBaseLineFrameNr, ArrayT< ArrayT<char> >& OutDatas) const;

    // Schreibt eine SC1_EntityUpdate Message nach 'OutData'.
    // Dabei wird der gegenwärtige 'Entity->State' gegen die BaseLine Delta-komprimiert, falls 'SendFromBaseLine==true',
    // ansonsten gegen den Zustand des Frames mit der Nummer 'ClientFrameNr'.
    // Wenn der Entity-State sich überhaupt nicht geändert hat und 'ForceInfo==false', wird überhaupt keine Nachricht generiert,
    // ansonsten zumindest der Header der SC1_EntityUpdate Message.
    // Gibt 'true' bei Erfolg zurück, sonst 'false'. Der einzige Möglichkeit, weswegen diese Funktion scheitern kann ('false'),
    // ist 'SendFromBaseLine==false' und eine zu kleine 'ClientFrameNr' (verlangt eine Komprimierung gegen etwas, was wir nicht (mehr) haben).
    // Im Falle des Scheiterns bleibt 'OutData' unberührt.
    bool WriteDeltaEntity(bool SendFromBaseLine, unsigned long ClientFrameNr, NetDataT& OutData, bool ForceInfo) const;


    /*******************/
    /*** Client Side ***/
    /*******************/

    // This creates a new EngineEntityT by taking a IntrusivePtrT<GameEntityI>, which previously must have been properly constructed from
    // the former parts of the SC1_EntityBaseLine in InData. It then fully constructs it by updating its non-initialized 'Entity_->State'
    // with the rest of the SC1_EntityBaseLine message.
    EngineEntityT(IntrusivePtrT<cf::GameSys::EntityT> Ent, NetDataT& InData);

    // Ausgehend vom (alten) Zustand des Frames 'DeltaFrameNr' wird der Entity Zustand des (neuen) Frames 'ServerFrameNr' bestimmt,
    // wobei Delta-Informationen anhand der DeltaMessage eingebracht werden.
    // 'DeltaFrameNr' ist die Nummer des Frames, gegen dessen Zustand Delta-dekomprimiert werden soll.
    // Ist dieser Parameter 0, so wird angenommen, daß DeltaMessage gegen die BaseLine angewandt werden sollen!
    // Gibt 'true' bei Erfolg zurück, sonst (Scheitern) 'false'.
    // Die Funktion scheitert bei "unpassenden" Parametern (wenn 'DeltaFrameNr<=EntityStateFrameNr<ServerFrameNr' verletzt ist)
    // und wenn 'DeltaFrameNr' zu alt ist (Versuch, gegen etwas zu Dekomprimieren, was wir nicht (mehr) haben).
    // Im Falle des Scheitersn bleibt die EngineEntityT Instanz unberührt.
    bool ParseServerDeltaUpdateMessage(unsigned long DeltaFrameNr, unsigned long ServerFrameNr, const ArrayT<uint8_t>* DeltaMessage);

    /// Updates the (predicted) state of this entity according to `PrevPlayerCommand` and `PlayerCommand`.
    void Predict(const PlayerCommandT& PrevPlayerCommand, const PlayerCommandT& PlayerCommand);


    private:

    EngineEntityT(const EngineEntityT&);            ///< Use of the Copy    Constructor is not allowed.
    void operator = (const EngineEntityT&);         ///< Use of the Assignment Operator is not allowed.

    /// Returns the serialized state of our Entity.
    cf::Network::StateT GetState() const;

    /// Sets the state of our Entity to the given State.
    /// @param State       The state to assign to the entity.
    /// @param IsIniting   Used to indicate that the call is part of the construction / first-time
    ///     initialization of the entity. The implementation will use this to not wrongly process
    ///     the event counters or to interpolate from stale values.
    void SetState(const cf::Network::StateT& State, bool IsIniting=false) const;


    IntrusivePtrT<cf::GameSys::EntityT> m_Entity;           ///< The game entity. On the client, it is in the most recent state as received from the server, *plus* any extrapolations (NPCs) and predictions (local human player) that are applied until the next update arrives.

    unsigned long                       EntityStateFrameNr; ///< `== ServerFrameNr` (the state number of Entity->State), used both on Client and Server side.

    cf::Network::StateT                 m_BaseLine;         ///< State of the entity immediately after it was created.
    const unsigned long                 m_BaseLineFrameNr;  ///< Frame number on which the entity was created. Only used on the server, unused on the client.
    ArrayT<cf::Network::StateT>         m_OldStates;        ///< States of the last n (server) frames, kept on both client and server side for delta compression.
};

#endif
