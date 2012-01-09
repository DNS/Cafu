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

#include "wx/wx.h"
#include "MapHelper.hpp"

#include "TypeSys.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapHelperT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapHelperT::TypeInfo(GetMapElemTIM(), "MapHelperT", "MapElementT", MapHelperT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapHelperT::MapHelperT(const MapEntityT* ParentEntity)
    : MapElementT(wxColour(128, 128, 128)),
      m_ParentEntity(ParentEntity)
{
}


MapHelperT::MapHelperT(const MapHelperT& Helper)
    : MapElementT(Helper),
      m_ParentEntity(Helper.m_ParentEntity)
{
}


void MapHelperT::SetParentEntity(const MapEntityT* ParentEntity)
{
    m_ParentEntity=ParentEntity;
}


void MapHelperT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapElementT::Assign(Elem);

    const MapHelperT* Helper=dynamic_cast<const MapHelperT*>(Elem);
    wxASSERT(Helper!=NULL);
    if (Helper==NULL) return;

    m_ParentEntity=Helper->m_ParentEntity;
}
