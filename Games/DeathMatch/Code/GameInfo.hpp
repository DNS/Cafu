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

#ifndef CAFU_DEATHMATCH_GAMEINFO_HPP_INCLUDED
#define CAFU_DEATHMATCH_GAMEINFO_HPP_INCLUDED

#include "../../GameInfo.hpp"

#ifndef GAME_NAME
#error Macro GAME_NAME must be defined!
#endif


namespace GAME_NAME
{
    /// This class provides the "DeathMatch" implementation of the GameInfoI interface.
    class GameInfoT : public cf::GameSys::GameInfoI
    {
        public:

        // Implement the methods of the GameInfoI interface.
        std::string GetName() const;
        cf::GameSys::GameI* CreateGame() const;
    };
}

#endif
