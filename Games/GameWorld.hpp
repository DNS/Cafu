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

#ifndef CAFU_GAMESYS_GAMEWORLD_INTERFACE_HPP_INCLUDED
#define CAFU_GAMESYS_GAMEWORLD_INTERFACE_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"


class BaseEntityT;
class CafuModelT;
namespace cf { namespace ClipSys { class ClipWorldT; } }
namespace cf { class UniScriptStateT; }


namespace cf
{
    namespace GameSys
    {
        /// The game world interface, specified as an ABC so that is can be used without linked (module-local) implementation.
        /// The engine provides each entity that it creates with a pointer to an implementation of this interface
        /// (as entities can be created on the client and the server side, there can be more than one implementation).
        /// See the GameI::CreateBaseEntityFrom*() methods for the "source" of these pointers:
        /// When one of these methods is called, a pointer to a GameWorldI is one of the parameters.
        class GameWorldI
        {
            public:

            /// The virtual destructor, so that derived classes can safely be deleted via a GameWorldI (base class) pointer.
            /// However, with GameWorldIs that's never supposed to happen, so this destructor only exists to silence the g++ compiler warning.
            virtual ~GameWorldI() { }

            /// Returns the clip world for the game world.
            virtual cf::ClipSys::ClipWorldT& GetClipWorld()=0;

            /// Returns the physics world for the game world.
            virtual PhysicsWorldT& GetPhysicsWorld()=0;

            /// Returns the script state for the game world.
            virtual cf::UniScriptStateT& GetScriptState()=0;

            /// Returns a "good" ambient light color for an arbitrary object (i.e. a model) of size Dimensions at Origin.
            /// The return value is derived from the worlds lightmap information "close" to the Dimensions at Origin.
            virtual Vector3fT GetAmbientLightColorFromBB(const BoundingBox3T<double>& Dimensions, const VectorT& Origin) const=0;

            /// Returns (a reference to) an array that contains the IDs of all entities that currently exist in the world.
            virtual const ArrayT<unsigned long>& GetAllEntityIDs() const=0;

            /// Returns a pointer to the entity with ID 'EntityID'.
            /// NULL is returned if that entity does not exist.
            virtual BaseEntityT* GetBaseEntityByID(unsigned long EntityID) const=0;

            /// Creates a new entity from the given parameters.
            /// The parameters are essentially what is also present in an editor map file.
            /// Returns the ID of the new entity on success, 0xFFFFFFFF on failure.
            virtual unsigned long CreateNewEntity(const std::map<std::string, std::string>& Properties, unsigned long CreationFrameNr, const VectorT& Origin)=0;

            /// Removes the entity identified by 'EntityID' from the (server) world.
            virtual void RemoveEntity(unsigned long EntityID)=0;

            /// Returns a model for the given filename.
            /// The returned model instance is managed by the GameWorldI implementation in a ModelManagerT,
            /// thus the caller does not have to (and if fact, must not) delete the CafuModelT instance.
            virtual const CafuModelT* GetModel(const std::string& FileName) const=0;
        };
    }
}

#endif
