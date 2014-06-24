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

#ifndef CAFU_GAMESYS_ENTITY_CREATE_PARAMS_HPP_INCLUDED
#define CAFU_GAMESYS_ENTITY_CREATE_PARAMS_HPP_INCLUDED

#include "TypeSys.hpp"


namespace cf
{
    namespace GameSys
    {
        class WorldT;


        /// Creation parameters for a game entity.
        class EntityCreateParamsT : public cf::TypeSys::CreateParamsT
        {
            public:

            /// The constructor.
            /// @param World_   The world in which the entity should be created.
            EntityCreateParamsT(WorldT& World_);

            /// This method is used on the client to create entities with the ID sent from the server,
            /// not the automatically created ID that would otherwise (normally) be used.
            void ForceID(unsigned int ID);

            /// Returns the "forced" ID that is to be used for the new entity,
            /// or `UINT_MAX` if the normal ID should be used.
            unsigned int GetID() const { return m_ID; }


            WorldT& World;  ///< The world in which the entity should be created.


            private:

            unsigned int m_ID;
        };
    }
}

#endif
