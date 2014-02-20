/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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

#ifndef CAFU_ENTITY_CREATE_PARAMS_HPP_INCLUDED
#define CAFU_ENTITY_CREATE_PARAMS_HPP_INCLUDED

#include "TypeSys.hpp"
#include "Math3D/Vector3.hpp"
#include "GameSys/Entity.hpp"
#include <map>


namespace cf { namespace GameSys { class GameWorldI; } }
namespace cf { namespace SceneGraph { class GenericNodeT; } }


namespace GAME_NAME
{
    class EntityCreateParamsT : public cf::TypeSys::CreateParamsT
    {
        public:

        EntityCreateParamsT(
            const unsigned long                       ID_,
            IntrusivePtrT<cf::GameSys::EntityT>       Entity_,
            const std::map<std::string, std::string>& Properties_,
            const cf::SceneGraph::GenericNodeT*       RootNode_,
            cf::GameSys::GameWorldI*                  GameWorld_);


        const unsigned long                       ID;
        IntrusivePtrT<cf::GameSys::EntityT>       Entity;
        const std::map<std::string, std::string>& Properties;
        const cf::SceneGraph::GenericNodeT*       RootNode;
        cf::GameSys::GameWorldI*                  GameWorld;
    };
}

#endif
