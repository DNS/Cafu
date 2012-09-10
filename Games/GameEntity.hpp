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


namespace cf { namespace TypeSys { class TypeInfoT; } }


/// This is the interface that the client and server use to access and work with game entities.
/// It is the only means by which the Cafu engine "knows" the game entities.
///
/// When the Cafu engine (client or server) needs a game entity, it uses the GameI interface to ask the game
/// implementation to create one (it is solely up to the game implementation to create concrete instances).
class GameEntityI : public RefCountedT
{
    public:

    /// Returns the proper type info for this entity.
    virtual const cf::TypeSys::TypeInfoT* GetType() const=0;

    /// Returns the (map unique) ID of this entity.
    virtual unsigned long GetID() const=0;

    /// Returns the origin point of this entity. Used for
    ///   - obtaining the camera position of the local human player entity (1st person view),
    ///   - computing light source positions.
    virtual const Vector3dT& GetOrigin() const=0;

    /// Returns the dimensions of this entity.
    virtual const BoundingBox3dT& GetDimensions() const=0;

    /// The virtual destructor.
    virtual ~GameEntityI() { }
};

#endif
