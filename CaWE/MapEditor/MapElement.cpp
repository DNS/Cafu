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

#include "MapElement.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "CompMapEntity.hpp"
#include "Group.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "MapPrimitive.hpp"

#include "TypeSys.hpp"
#include "wx/wx.h"


using namespace MapEditor;


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


MapElementT::MapElementT()
    : m_Parent(NULL),
      m_IsSelected(false),
      m_Group(NULL),
      m_FrameCount(0)
{
}


MapElementT::MapElementT(const MapElementT& Elem)
    : m_Parent(NULL),
      m_IsSelected(false),    // The copied element cannot initially be selected: It is not a member of the selection array (kept in the map document).
      m_Group(NULL),
      m_FrameCount(Elem.m_FrameCount)
{
}


// We cannot define this destructor inline in the "MapElement.hpp" header file,
// because the (Visual C++) compiler would then expect us to #include the "CompMapEntity.hpp" header
// file in quasi every .cpp file in the application (or in "MapElement.hpp").
// I don't know exactly why this is so, but obviously the inline definition triggers the instantiation
// of the IntrusivePtrT<CompMapEntityT>, which in the header file is only forward-declared.
MapElementT::~MapElementT()
{
}


void MapElementT::SetParent(CompMapEntityT* Ent)
{
    m_Parent = Ent;
}


bool MapElementT::IsVisible() const
{
    return !m_Group || m_Group->IsVisible;
}


bool MapElementT::CanSelect() const
{
    return !m_Group || m_Group->CanSelect;
}


void MapElementT::GetToggleEffects(ArrayT<MapElementT*>& RemoveFromSel, ArrayT<MapElementT*>& AddToSel, bool AutoGroupEntities)
{
    IntrusivePtrT<CompMapEntityT> Entity = GetParent();

    // If this element belongs to a non-world entity, put all elements of the entity into the appropriate lists.
    if (!Entity->IsWorld() && AutoGroupEntities)
    {
        const ArrayT<MapElementT*> AllElems = Entity->GetAllMapElements();

        for (unsigned long ElemNr = 0; ElemNr < AllElems.Size(); ElemNr++)
        {
            MapElementT* Elem = AllElems[ElemNr];

            // Insert Elem into one of the lists, but only if it isn't mentioned there already.
            if (RemoveFromSel.Find(Elem) == -1 && AddToSel.Find(Elem) == -1)
            {
                if (Elem->IsSelected()) RemoveFromSel.PushBack(Elem);
                                   else AddToSel.PushBack(Elem);
            }
        }
    }

    // If this element is a member of a group, put all members of the group into the appropriate lists.
    if (m_Group && m_Group->SelectAsGroup)
    {
        const ArrayT<MapElementT*> GroupMembers = m_Group->GetMembers();

        for (unsigned long MemberNr = 0; MemberNr < GroupMembers.Size(); MemberNr++)
        {
            MapElementT* Member = GroupMembers[MemberNr];

            // Insert Member into one of the lists, but only if it isn't mentioned there already.
            if (RemoveFromSel.Find(Member) == -1 && AddToSel.Find(Member) == -1)
            {
                if (Member->IsSelected()) RemoveFromSel.PushBack(Member);
                                     else AddToSel.PushBack(Member);
            }
        }
    }

    // Finally insert this element itself into one of the lists, but only if it isn't mentioned there already.
    if (RemoveFromSel.Find(this) == -1 && AddToSel.Find(this) == -1)
    {
        if (this->IsSelected()) RemoveFromSel.PushBack(this);
                           else AddToSel.PushBack(this);
    }
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
