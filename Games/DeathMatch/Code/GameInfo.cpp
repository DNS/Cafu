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

#include "GameInfo.hpp"
#include "GameImpl.hpp"

#define QUOTE(str) QUOTE_HELPER(str)
#define QUOTE_HELPER(str) #str

using namespace GAME_NAME;


std::string GameInfoT::GetName() const
{
    return QUOTE(GAME_NAME);
}


cf::GameSys::GameI* GameInfoT::CreateGame() const
{
    return &GameImplT::GetInstance();
}
