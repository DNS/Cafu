/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "MapPrimitive.hpp"
#include "MapEntity.hpp"
#include "TypeSys.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapPrimitiveT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapPrimitiveT::TypeInfo(GetMapElemTIM(), "MapPrimitiveT", "MapElementT", MapPrimitiveT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapPrimitiveT::MapPrimitiveT(const wxColour& Color)
    : MapElementT(Color),
      m_Parent(NULL)
{
}


MapPrimitiveT::MapPrimitiveT(const MapPrimitiveT& Prim)
    : MapElementT(Prim),
      m_Parent(NULL)
{
}


void MapPrimitiveT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapElementT::Assign(Elem);

    // const MapPrimitiveT* Prim=dynamic_cast<const MapPrimitiveT*>(Elem);
    // wxASSERT(Prim!=NULL);
    // if (Prim==NULL) return;

    // m_Parent=...;       // Not changed by this method, per definition.
}


wxColour MapPrimitiveT::GetColor(bool ConsiderGroup) const
{
    if (m_Group && ConsiderGroup)
        return m_Group->Color;

    if (m_Parent && m_Parent->GetType()==&MapEntityT::TypeInfo)
        return m_Parent->GetColor(false);

    // The primitive has no parent, or the parent is the world.
    return m_Color;
}
