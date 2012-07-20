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

#ifndef CAFU_GAME_GAMEINTERFACE_HPP_INCLUDED
#define CAFU_GAME_GAMEINTERFACE_HPP_INCLUDED

#include "Templates/Pointer.hpp"

#include <map>
#include <string>


// Forward declarations.
class BaseEntityT;
template<class T> class Vector3T;
class ModelManagerT;
namespace cf { namespace ClipSys { class CollisionModelT; } }
namespace cf { namespace GameSys { class GameWorldI; } }
namespace cf { namespace SceneGraph { class GenericNodeT; } }
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }


namespace cf
{
    namespace GameSys
    {
        /// The game interface, specified as an ABC so that is can be used without linked (module-local) implementation.
        /// This interface is implemented by the game DLL, and thus defines the essence of a game/MOD.
        /// It is used (only) by the engine to run the game.
        class GameI
        {
            public:

            /// Called to initialize the game.
            /// This function is called exactly once as the very first function after the game has been loaded,
            /// not each time a new world is about to be loaded.
            /// @param AsClient     Tells whether we're running as client.
            /// @param AsServer     Tells whether we're running as server.
            /// @param ModelMan     The manager for all models that are used in this game.
            virtual void Initialize(bool AsClient, bool AsServer, ModelManagerT& ModelMan)=0;

            /// Called to shutdown the game.
            /// This function is called exactly once as the very last function before the game is released,
            /// not each time a world is being freed.
            virtual void Release()=0;


            /// Returns the type information manager with the entity hierarchy.
            virtual const cf::TypeSys::TypeInfoManT& GetEntityTIM() const=0;

            /// Creates a new entity of the given type.
            /// @param TI             The type of entity to create.
            /// @param Properties     The properties dictionary in the map file of this entity (empty if the entity is created "dynamically"). Contains especially the "classname" key that was used by the caller to determine TI.
            /// @param RootNode       The root node of the scene graph of this entity as defined in the map file. NULL if the entity is created "dynamically".
            /// @param CollisionModel The collision model of this entity as defined by map primitives. NULL if no collision model was defined by map primitives (the entity may still have a collision model based on the Properties).
            /// @param ID             The global unique ID of this entity (in the current map).
            /// @param WorldFileIndex The index number of the entity in the world file.
            /// @param MapFileIndex   The index number of the entity in the map file, for obtaining the information in the map file about it later.
            /// @param GameWorld      Pointer to the game world implementation.
            /// @param Origin         Where the new entity is supposed to be instantiated.
            /// @returns a base class pointer to the newly created entity instance.
            virtual IntrusivePtrT<BaseEntityT> CreateBaseEntityFromMapFile(const cf::TypeSys::TypeInfoT* TI, const std::map<std::string, std::string>& Properties,
                const cf::SceneGraph::GenericNodeT* RootNode, const cf::ClipSys::CollisionModelT* CollisionModel, unsigned long ID,
                unsigned long WorldFileIndex, unsigned long MapFileIndex, cf::GameSys::GameWorldI* GameWorld, const Vector3T<double>& Origin)=0;

            /// Creates a new entity of the given type (specified by TypeNr), and returns a pointer to the base class.
            /// This method is used by the client after it received a related network message (which also contains the TypeNr).
            /// @param TypeNr         The (number of the) type of the entity to be created.
            /// @param Properties     The properties dictionary in the map file of this entity (empty if the entity is created "dynamically").
            /// @param RootNode       The root node of the scene graph of this entity as defined in the map file. NULL if the entity is created "dynamically".
            /// @param CollisionModel The collision model of this entity as defined by map primitives. NULL if no collision model was defined by map primitives (the entity may still have a collision model based on the Properties).
            /// @param ID             The global unique ID of this entity (in the current map).
            /// @param WorldFileIndex The index number of the entity in the world file.
            /// @param MapFileIndex   The index number of the entity in the map file, for obtaining the information in the map file about it later.
            /// @param GameWorld      Pointer to the game world implementation.
            /// @returns a base class pointer to the newly created entity instance.
            virtual IntrusivePtrT<BaseEntityT> CreateBaseEntityFromTypeNr(unsigned long TypeNr, const std::map<std::string, std::string>& Properties,
                const cf::SceneGraph::GenericNodeT* RootNode, const cf::ClipSys::CollisionModelT* CollisionModel, unsigned long ID,
                unsigned long WorldFileIndex, unsigned long MapFileIndex, cf::GameSys::GameWorldI* GameWorld)=0;

            /// The virtual destructor, so that derived classes can safely be deleted via a GameI (base class) pointer.
            /// However, with GameIs that's never supposed to happen, so this destructor only exists to silence the g++ compiler warning.
            virtual ~GameI() { }
        };
    }
}

#endif
