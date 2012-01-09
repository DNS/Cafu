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

#ifndef CAFU_GUISYS_COROUTINES_HPP_INCLUDED
#define CAFU_GUISYS_COROUTINES_HPP_INCLUDED

#include "Templates/Array.hpp"


struct lua_State;


/// DOCTODO
struct CoroutineT
{
    /// Constructor.
    CoroutineT();

    unsigned long ID;           ///< The unique ID of this coroutine, used to anchor it in a table in the Lua registry. Automatically set in the constructor, but not declared const so that CoroutineT objects can be kept in an ArrayT<>.
    lua_State*    State;        ///< The state and stack of this coroutine.
    unsigned long NumParams;    ///< Number of parameters on the stack of State for the next call to lua_resume(), i.e. the parameters for the initial function call or the return values for the pending yield().
    float         WaitTimeLeft; ///< Wait time left until the next call to lua_resume().
};


/// DOCTODO
class CoroutineManT
{
    public:

    ArrayT<CoroutineT> PendingCoroutines;   ///< The list of active, pending coroutines.
};

#endif
