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

#ifndef CAFU_ENGINEENTITY_HPP_INCLUDED
#define CAFU_ENGINEENTITY_HPP_INCLUDED

#include "../Games/BaseEntity.hpp"
#include "../Games/PlayerCommand.hpp"
#include "Math3D/Vector3.hpp"
#include "Network/State.hpp"
#include "Templates/Pointer.hpp"

class BaseEntityT;
class NetDataT;


class EngineEntityT
{
    public:

    /******************/
    /*** Both Sides ***/
    /******************/

    ~EngineEntityT();

    // Rückt einen Zeiger auf unseren BaseEntity heraus.
    // Hauptsächlich gedacht für die Aufrufkette "Konkreter Entity --> ESInterface --> EntityManager --> hier".
    // Somit kann sich ein Entity beim Thinken z.B. Überblick über andere Entities verschaffen.
    IntrusivePtrT<BaseEntityT> GetBaseEntity() const;

    // Bearbeitet den 'ConfigString' und die 'ConfigData'.
    void ProcessConfigString(const void* ConfigData, const char* ConfigString);


    /*******************/
    /*** Server Side ***/
    /*******************/

    // Creates a new EngineEntityT, where 'Entity_' points to a formerly constructed BaseEntityT (with initial state properly stored in Entity_->State)
    // and 'CreationFrameNr' is the number of the server frame for which this EngineEntityT is created.
    // Server side EngineEntityT creation is always (and only) triggered by an EntityManagerT, either after a new world was loaded or
    // when a BaseEntityT creates a new entity by calling GameWorld->CreateNewEntity() in its 'Think()' method.
    EngineEntityT(IntrusivePtrT<BaseEntityT> Entity_, unsigned long CreationFrameNr);

    // Prepares the entity to enter the next state for frame 'ServerFrameNr'.
    // This function must be called for each entity before any entities 'Think()' function is called.
    // Reason: Any thinking entity may also (implicitly) modify *other* entities, e.g. by calling their 'TakeDamage()' functions.
    // If such entities had not been previously prepared by this function, the implementation could not keep the state management right.
    void PreThink(unsigned long ServerFrameNr);

    // Calculates the new state for the next frame 'ServerFrameNr', where 'FrameTime' seconds passed since the previous frame.
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

    // This creates a new EngineEntityT by taking a IntrusivePtrT<BaseEntityT>, which previously must have been properly constructed from
    // the former parts of the SC1_EntityBaseLine in InData. It then fully constructs it by updating its non-initialized 'Entity_->State'
    // with the rest of the SC1_EntityBaseLine message.
    EngineEntityT(IntrusivePtrT<BaseEntityT> Entity_, NetDataT& InData);

    // Ausgehend vom (alten) Zustand des Frames 'DeltaFrameNr' wird der Entity Zustand des (neuen) Frames 'ServerFrameNr' bestimmt,
    // wobei Delta-Informationen anhand der DeltaMessage eingebracht werden.
    // 'DeltaFrameNr' ist die Nummer des Frames, gegen dessen Zustand Delta-dekomprimiert werden soll.
    // Ist dieser Parameter 0, so wird angenommen, daß DeltaMessage gegen die BaseLine angewandt werden sollen!
    // Gibt 'true' bei Erfolg zurück, sonst (Scheitern) 'false'.
    // Die Funktion scheitert bei "unpassenden" Parametern (wenn 'DeltaFrameNr<=EntityStateFrameNr<ServerFrameNr' verletzt ist)
    // und wenn 'DeltaFrameNr' zu alt ist (Versuch, gegen etwas zu Dekomprimieren, was wir nicht (mehr) haben).
    // Im Falle des Scheitersn bleibt die EngineEntityT Instanz unberührt.
    bool ParseServerDeltaUpdateMessage(unsigned long DeltaFrameNr, unsigned long ServerFrameNr, const ArrayT<uint8_t>* DeltaMessage);

    // This function is called after we received any in-game message (which should always contain an SC1_FrameInfo message) from the server.
    // From such a message we know that the server has seen all PlayerCommands up to the 'RemoteLastIncomingSequenceNr' packet,
    // and that all entities (especially this one!) are in the corresponding state.
    // (The number 'RemoteLastIncomingSequenceNr' is provided by the network protocol and must be handed-in here.)
    // Consequently, this function applies all PlayerCommands from 'RemoteLastIncomingSequenceNr+1' to 'LastOutgoingSequenceNr'
    // to the current state, and puts the result in the 'PredictedState' of this entity.
    // Returns 'true' on success, 'false' on failure. The functions fails when the RemoteLastIncomingSequenceNr becomes too old.
    bool Repredict(const ArrayT<PlayerCommandT>& PlayerCommands, unsigned long RemoteLastIncomingSequenceNr, unsigned long LastOutgoingSequenceNr);

    // Updates the predicted state of this entity according to the 'PlayerCommand'.
    // The 'OutgoingSequenceNr' is the sequence number of the packet in which the caller sends the 'PlayerCommand' to the server.
    void Predict(const PlayerCommandT& PlayerCommand, unsigned long OutgoingSequenceNr);

    /// Returns the camera details of this entity that the client should use to render the world.
    /// This is typically called for the local human player from whose perspective the world is rendered.
    /// @param UsePredictedState   Whether the predicted or the "unpredicted" state should provide the camera details.
    void GetCamera(bool UsePredictedState, Vector3dT& Origin, unsigned short& Heading, unsigned short& Pitch, unsigned short& Bank) const;

    // Returns the light source info for this entity. If UsePredictedState is true, the light source info for the predicted entity is returned.
    bool GetLightSourceInfo(bool UsePredictedState, unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const;

    // Draws this entity. The Ca3DEWorld parameter is only required so that we can learn about this entities' ambient color.
    void Draw(bool FirstPersonView, bool UsePredictedState, const VectorT& ViewerPos) const;

    // Calls the 'PostDraw()' function of this entity.
    void PostDraw(float FrameTime, bool FirstPersonView, bool UsePredictedState);


    private:

    EngineEntityT(const EngineEntityT&);            ///< Use of the Copy    Constructor is not allowed.
    void operator = (const EngineEntityT&);         ///< Use of the Assignment Operator is not allowed.

    /// Returns the serialized state of our Entity.
    cf::Network::StateT GetState() const;

    /// Sets the state of our Entity to the given State.
    /// @param State       The state to assign to the entity.
    /// @param IsIniting   Used to indicate that the call is part of the construction / first-time
    ///     initialization of the entity. The implementation will use this to not wrongly process
    ///     the event counters.
    void SetState(const cf::Network::StateT& State, bool IsIniting=false) const;


    IntrusivePtrT<BaseEntityT>  Entity;             // Base entity as allocated via the cf::GameSys::Game interface.

    unsigned long               EntityStateFrameNr; // ==ServerFrameNr (the state number of Entity->State), used both on Client and Server side

    cf::Network::StateT         m_BaseLine;         ///< State of the entity immediately after it was created.
    unsigned long               BaseLineFrameNr;    ///< Frame number on which the entity was created.
    ArrayT<cf::Network::StateT> m_OldStates;        ///< States of the last n (server) frames, kept on both client and server side for delta compression.
    cf::Network::StateT         m_PredictedState;   ///< The current predicted state.

    // Variables for interpolating the origin of non-predicted entities (i.e. all but the local player entity).
    bool                        m_Interpolate_Ok;   ///< Is interpolation currently possible? (false e.g. when the last update of the Entity->State was not relative to one of the OldStates).
    Vector3dT                   m_InterpolateOrigin0;///< If Interpolate_Ok, this is the previous entity origin to interpolate *from* (from the most recent OldStates). The current entity state to interpolate *to* is the usual Entity->State.
    double                      m_InterpolateTime0; ///< The clients global time at which the InterpolateState0 was received.
    double                      m_InterpolateTime1; ///< The clients global time at which the Entity->State     was received.
};

#endif
