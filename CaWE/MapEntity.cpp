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

#include "MapEntity.hpp"
#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "MapPrimitive.hpp"
#include "Options.hpp"

#include "TypeSys.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapEntityT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapEntityT::TypeInfo(GetMapElemTIM(), "MapEntityT", "MapEntityBaseT", MapEntityT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapEntityT::MapEntityT(MapDocumentT& MapDoc)
    : MapEntityBaseT(MapDoc)
{
}


MapEntityT::MapEntityT(const MapEntityT& Entity)
    : MapEntityBaseT(Entity)
{
}


MapEntityT* MapEntityT::Clone() const
{
    return new MapEntityT(*this);
}


/// Entities dilemma: What should the bounding-box of an entity comprise?
///    a) Only its origin (plus some margin)?
///    b) Its origin plus all of its primitives children?
///
/// This looks very much like a dilemma, because a) seems useful in a few cases (e.g. when an entity - i.e. its origin,
/// but none of its primitives - is moved and the observers want to determine the smallest required screen update region),
/// but b) seems to be required in at least as many cases.
///
/// The solution to the dilemma is to fix the assumptions: It's wrong to think about an entity as just its origin point.
/// Instead, we think about an entity always as the whole. That is, b) is the proper choice:
/// The size of an entity always equates to a bounding-box that comprises its origin and *all of its primitives*.
/// For example, the visual represenation of an entity in the 2D views is a rectangle that outlines that bounding-box.
/// Note however that this property is entirely *unrelated* to the mechanics of selection, where an entity and its
/// primitives are treated independently as if they had no relationship at all!
BoundingBox3fT MapEntityT::GetBB() const
{
    BoundingBox3fT BB;

    if (!m_Class->IsSolidClass()) BB+=m_Origin;

    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        BB+=m_Primitives[PrimNr]->GetBB();

    // for (unsigned long HelperNr=0; HelperNr<m_Helpers.Size(); HelperNr++)
    //     BB+=m_Helpers[HelperNr]->GetBB();

    if (!BB.IsInited()) BB+=m_Origin;
    return BB;
}
