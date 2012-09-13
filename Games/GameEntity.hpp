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

#ifndef CAFU_GAME_ENTITY_HPP_INCLUDED
#define CAFU_GAME_ENTITY_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace GameSys { class GameWorldI; } }
namespace cf { namespace Network { class InStreamT; } }
namespace cf { namespace Network { class OutStreamT; } }
namespace cf { namespace TypeSys { class TypeInfoT; } }


/// This is the interface that the client and server use to access and work with game entities.
/// It is the only means by which the Cafu engine "knows" the game entities.
///
/// When the Cafu engine (client or server) needs a game entity, it uses the GameI interface to ask the game
/// implementation to create one: it is solely up to the game implementation to create concrete instances.
class GameEntityI : public RefCountedT
{
    public:

    /// The virtual destructor.
    virtual ~GameEntityI() { }

    /// Returns the proper type info for this entity.
    virtual const cf::TypeSys::TypeInfoT* GetType() const=0;

    /// Returns the (map unique) ID of this entity.
    virtual unsigned long GetID() const=0;

    /// Returns the map name of this entity.
    virtual std::string GetName() const=0;

    /// Returns the world file index of this entity.
    virtual unsigned long GetWorldFileIndex() const=0;

    /// Returns the game world that this entity is in.
    virtual cf::GameSys::GameWorldI* GetGameWorld() const=0;

    /// Returns the origin point of this entity. Used for
    ///   - obtaining the camera position of the local human player entity (1st person view),
    ///   - computing light source positions.
    virtual const Vector3dT& GetOrigin() const=0;

    /// Returns the dimensions of this entity.
    virtual const BoundingBox3dT& GetDimensions() const=0;

    /// Returns the camera orientation angles of this entity.
    /// Used for setting up the camera of the local human player entity (1st person view).
    virtual void GetCameraOrientation(unsigned short& h, unsigned short& p, unsigned short& b) const=0;

    /// Returns the orientation angles of the entity itself.
    /// Used for computing the light source and eye positions in entity (model) space.
    /// TODO: Both the signature as well as the implementation of this method are temporary,
    ///       and fully expected to change later.
    virtual void GetBodyOrientation(unsigned short& h, unsigned short& p, unsigned short& b) const=0;

    /// Writes the current state of this entity into the given stream.
    /// This method is called to send the state of the entity over the network or to save it to disk.
    ///
    /// The implementation calls DoSerialize(), that derived classes override to add their own data.
    ///
    /// Note that this method is the twin of Deserialize(), whose implementation it must match.
    virtual void Serialize(cf::Network::OutStreamT& Stream) const=0;

    /// Reads the state of this entity from the given stream, and updates the entity accordingly.
    /// This method is called after the state of the entity has been received over the network,
    /// has been loaded from disk, or must be "reset" for the purpose of (re-)prediction.
    ///
    /// The implementation calls DoDeserialize(), that derived classes override to read their own data.
    /// It also calls ProcessEvent() (overridden by derived classes) for any received events.
    ///
    /// Note that this method is the twin of Serialize(), whose implementation it must match.
    ///
    /// @param Stream
    ///   The stream to read the state data from.
    ///
    /// @param IsIniting
    ///   Used to indicate that the call is part of the construction / first-time initialization of the entity.
    ///   The implementation will use this to not wrongly process the event counters, interpolation, etc.
    virtual void Deserialize(cf::Network::InStreamT& Stream, bool IsIniting)=0;


    /// THIS FUNCTION IS DEPRECATED! TRY TO AVOID TO USE IT!
    /// This DEPRECATED, SERVER-SIDE function is called in order to "communicate" with this entity.
    /// It is probably a (bad) replacement for additional, missing member functions, so don't use it!
    /// Mostly used for setting/querying the properties of specific, concrete entities,
    /// e.g. the player names, player model names, player commands, or if something is solid,
    /// alive or was picked up (and is thus "invisible" for a while), and so on.
    /// This should most certainly be resolved as soon as possible, requires careful design considerations, however.
    /// At a first glance, the related methods are *only* called from the server (like passing in player commands),
    /// or from within the Think() functions, but never on the client side.
    virtual void ProcessConfigString(const void* ConfigData, const char* ConfigString)=0;

    /// This SERVER-SIDE function is called by the server in order to advance the world one clock-tick.
    /// That is, basing on the present (old) state, it is called for computing the next (new) state.
    /// 'FrameTime' is the time of the clock-tick, in seconds.
    /// 'ServerFrameNr' is the number of the current server frame.
    /// >>> IMPORTANT NOTE: In truth, also the CLIENT-SIDE calls this function for the purpose of predicting the local human player entity! <<<
    /// >>> As a consequence, special rules apply when this function is called for predicted entities (that is, human player entities).     <<<
    /// >>> For further details and examples, please refer to the EntHumanPlayerT::Think() function in HumanPlayer.cpp.                     <<<
    virtual void Think(float FrameTime, unsigned long ServerFrameNr)=0;


    /// This CLIENT-SIDE function is called in order to retrieve light source information about this entity.
    /// Returns 'true' if this entity is a light source and 'DiffuseColor', 'SpecularColor',
    /// 'Position' (in world space!), 'Radius' and 'CastsShadows' have been set.
    /// Returns 'false' if this entity is no light source.
    /// (In theory, this function might also be called on server-side, from within Think().)
    virtual bool GetLightSourceInfo(unsigned long& DiffuseColor, unsigned long& SpecularColor, VectorT& Position, float& Radius, bool& CastsShadows) const=0;

    /// This CLIENT-SIDE function is called by the client in order to get this entity drawn.
    ///
    /// Note that it is usually called several times per frame, in order to gather the individual terms of the
    /// lighting equation: there is a call for the ambient contribution (everything that is independent of a
    /// lightsource) and for each light source there is a call for the shadows, followed by another call for
    /// adding the lightsource-dependent contribution (diffuse and specular terms etc.).
    ///
    /// Also note that the calling code has properly set up the Cafu Material Systems global lighting parameters
    /// before calling. That is, the ambient light color, light source position (in model space), radius, diff+spec
    /// color and the eye position (in model space) are set. However, normally only those parameters that are relevant
    /// for the current Material Systems rendering action are set: In the AMBIENT rendering action,
    /// only the ambient colors are set, in the STENCILSHADOW action only the light source position may be set,
    /// and in the LIGHTING action all parameters except for the ambient light color are set.
    ///
    /// @param FirstPersonView   is true when the engine has rendered the world from this entities viewpoint,
    ///                          e.g. because this is the local players entity.
    /// @param LodDist           is the world-space distance of the entity to the viewer,
    ///                          supposed to be used for level-of-detail reductions.
    virtual void Draw(bool FirstPersonView, float LodDist) const=0;

    /// This CLIENT-SIDE function is called by the client in order to advance all values of this entity that have
    /// been registered for interpolation.
    /// Each concrete entity class determines itself which values are interpolated (e.g. the m_Origin) by calling
    /// Register() in its constructor: see Register() for details.
    /// Interpolation is used to "smooth" values in the client video frames between server updates.
    virtual void Interpolate(float FrameTime)=0;

    /// This CLIENT-SIDE function is called once per frame, for each entity, after the regular rendering
    /// (calls to 'Draw()') is completed, in order to provide entities an opportunity to render the HUD,
    /// employ simple "mini-prediction", triggers sounds, register particles, do other server-independent eye-candy,
    /// and so on.
    ///
    /// @param FrameTime         is the time in seconds that it took to render the last (previous) frame.
    /// @param FirstPersonView   is true when the engine has rendered the world from this entities viewpoint,
    ///                          e.g. because this is the local players entity.
    ///
    /// As it is convenient for current code (dealing with the particle system...), it is guaranteed that there is
    /// exactly one call to this function with 'FirstPersonView==true', which occurs after the appropriate calls
    /// for all other entities. This behaviour may change in the future.
    virtual void PostDraw(float FrameTime, bool FirstPersonView)=0;
};

#endif
