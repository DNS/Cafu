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

#ifndef CAFU_GAME_GAMEINFO_HPP_INCLUDED
#define CAFU_GAME_GAMEINFO_HPP_INCLUDED

#include <string>


namespace cf
{
    namespace GameSys
    {
        class GameI;


        /// This interface provides early information about a game.
        ///
        /// The Cafu Engine can be linked to an arbitrary number of games (MODs), that are independent of each other.
        /// Each game must implement the GameInfoI interface, of which the Cafu Engine assembles a global list of
        /// known, available games. The list is typically used to let the user choose which game he wishes to play,
        /// and to obtain a GameI instance of the chosen game.
        class GameInfoI
        {
            public:

            /// Returns the name of this game.
            /// Among other purposes, the returned string is used in the client and server to construct file system
            /// paths to game resource files. Consequently, the returned name should only consist of a subset of
            /// characters that are valid for use in directory and file names.
            virtual std::string GetName() const=0;

            /// Creates and returns an instance of this game.
            virtual GameI* CreateGame() const=0;

            /// The virtual destructor, so that derived classes can safely be deleted via a GameInfoI (base class) pointer.
            virtual ~GameInfoI() { }
        };
    }
}

#endif
