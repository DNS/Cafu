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
#include "ChildFrameViewWin2D.hpp"
#include "MapElement.hpp"
#include "MapEntity.hpp"
#include "MapDocument.hpp"

#include "TypeSys.hpp"


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& GetMapElemTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


/*** Begin of TypeSys related definitions for this class. ***/

void* MapElementT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapElementT::TypeInfo(GetMapElemTIM(), "MapElementT", NULL, MapElementT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapElementT::MapElementT(const wxColour& Color)
    : m_IsSelected(false),
      m_Color(Color),
      m_Group(NULL),
      m_FrameCount(0)
{
}


MapElementT::MapElementT(const MapElementT& Elem)
    : m_IsSelected(false),    // The copied element cannot initially be selected: It is not a member of the selection array (kept in the map document).
      m_Color(Elem.m_Color),
      m_Group(NULL),
      m_FrameCount(Elem.m_FrameCount)
{
}


void MapElementT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

 // m_IsSelected=Elem->m_IsSelected;    // The selection status cannot be changed by the assignment: It is kept in the map document and up to the caller to change.
    m_Color     =Elem->m_Color;
 // m_Group     =...;                   // Not changed by this method, it's up to the caller to change.
 // m_FrameCount=Elem->m_FrameCount;
}


wxColour MapElementT::GetColor(bool ConsiderGroup) const
{
    if (m_Group && ConsiderGroup)
        return m_Group->Color;

    return m_Color;
}


bool MapElementT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    return GetBB().TraceRay(RayOrigin, RayDir, Fraction);
}


bool MapElementT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    // It is recommended that derived classes that override this method do *NOT* call this base class implementation, because
    //   a) this implementation is not specified nor guaranteed to act and behave as a quick bounding-box check, and
    //   b) GetBB() can be expensive (at the time of this writing) and might be called again in the overridden method.
    const wxRect         Disc=wxRect(Pixel, Pixel).Inflate(Radius, Radius);
    const BoundingBox3fT BB  =GetBB();

    // Determine if this map elements BB intersects the Disc (which is actually rectangular...).
    return wxRect(ViewWin.WorldToWindow(BB.Min), ViewWin.WorldToWindow(BB.Max)).Intersects(Disc);
}
