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

#include "CompMapEntity.hpp"
#include "MapEntityBase.hpp"
#include "GameSys/AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace MapEditor;


ComponentMapEntityT::ComponentMapEntityT(MapEntityBaseT* MapEnt)
    : ComponentBaseT(),
      m_MapEntity(MapEnt)
{
}


ComponentMapEntityT::ComponentMapEntityT(const ComponentMapEntityT& Comp)
    : ComponentBaseT(Comp),
      m_MapEntity(Comp.m_MapEntity ? new MapEntityBaseT(*Comp.m_MapEntity) : NULL)
{
}


ComponentMapEntityT::~ComponentMapEntityT()
{
    delete m_MapEntity;
}


ComponentMapEntityT* ComponentMapEntityT::Clone() const
{
    return new ComponentMapEntityT(*this);
}


void ComponentMapEntityT::Render() const
{
}
