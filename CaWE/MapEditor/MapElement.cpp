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
      m_FrameCount(0)
{
}


MapElementT::MapElementT(const MapElementT& Elem)
    : m_Parent(NULL),
      m_IsSelected(false),    // The copied element cannot initially be selected: It is not a member of the selection array (kept in the map document).
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


void MapElementT::Assign(const MapElementT* Elem)
{
    if (Elem == this) return;

 // m_Parent     = ...;                 // Not changed by this method, per definition.
 // m_IsSelected = Elem->m_IsSelected;  // The selection status cannot be changed by the assignment: It is kept in the map document and up to the caller to change.
 // m_Group      = ...;                 // Not changed by this method, it's up to the caller to change.
 // m_FrameCount = Elem->m_FrameCount;
}


void MapElementT::SetParent(CompMapEntityT* Ent)
{
    m_Parent = Ent;
}


bool MapElementT::IsVisible() const
{
    IntrusivePtrT<CompMapEntityT> Top = GetTopmostGroupSel();

    if (Top == NULL)
        return GetParent()->GetEntity()->GetBasics()->IsShown();

    return Top->GetEntity()->GetBasics()->IsShown();
}


bool MapElementT::CanSelect() const
{
    if (!IsVisible()) return false;

    return GetParent()->GetEntity()->GetBasics()->GetSelMode() != cf::GameSys::ComponentBasicsT::LOCKED;
}


IntrusivePtrT<CompMapEntityT> MapElementT::GetTopmostGroupSel() const
{
    IntrusivePtrT<CompMapEntityT> Top = NULL;

    // Bubble up to the topmost parent that is to be selected "as one" (as a group), if there is one.
    for (IntrusivePtrT<cf::GameSys::EntityT> Ent = GetParent()->GetEntity(); Ent != NULL; Ent = Ent->GetParent())
        if (Ent->GetBasics()->GetSelMode() == cf::GameSys::ComponentBasicsT::GROUP)
            Top = GetMapEnt(Ent);

    return Top;
}


namespace
{
    /// An auxiliary method for GetToggleEffects().
    /// It computes the toggle effects for the given MapEnt, all its primitives and all its children recursively.
    void GetToggleEffectsRecursive(IntrusivePtrT<CompMapEntityT> MapEnt, ArrayT<MapElementT*>& RemoveFromSel, ArrayT<MapElementT*>& AddToSel)
    {
        MapEntRepresT* Repres = MapEnt->GetRepres();

        // TODO: If hidden (e.g. child entities), hierarchically force the whole MapEnt to be *visible*!

        // Toggle the Repres by inserting it into one of the lists, but only if it isn't mentioned there already.
        if (RemoveFromSel.Find(Repres) == -1 && AddToSel.Find(Repres) == -1)
        {
            if (Repres->IsSelected()) RemoveFromSel.PushBack(Repres);
                                 else AddToSel.PushBack(Repres);
        }

        // Toggle the entities primitives analogously.
        for (unsigned long PrimNr = 0; PrimNr < MapEnt->GetPrimitives().Size(); PrimNr++)
        {
            MapPrimitiveT* Prim = MapEnt->GetPrimitives()[PrimNr];

            // Insert Member into one of the lists, but only if it isn't mentioned there already.
            if (RemoveFromSel.Find(Prim) == -1 && AddToSel.Find(Prim) == -1)
            {
                if (Prim->IsSelected()) RemoveFromSel.PushBack(Prim);
                                   else AddToSel.PushBack(Prim);
            }
        }

        // Recursively toggle the entity's children as well.
        IntrusivePtrT<cf::GameSys::EntityT> Ent = MapEnt->GetEntity();

        for (unsigned long ChildNr = 0; ChildNr < Ent->GetChildren().Size(); ChildNr++)
        {
            GetToggleEffectsRecursive(GetMapEnt(Ent->GetChildren()[ChildNr]), RemoveFromSel, AddToSel);
        }
    }
}


void MapElementT::GetToggleEffects(ArrayT<MapElementT*>& RemoveFromSel, ArrayT<MapElementT*>& AddToSel)
{
    IntrusivePtrT<CompMapEntityT> Top = GetTopmostGroupSel();

    if (Top != NULL)
    {
        // If the element belongs to an entity that is to be selected "as one" (as a group),
        // put all of the entity's parts into the appropriate lists.
        if (false)
        {
            // This works, but if Top is already partially selected, it would result is a piece-wise toggle.
            GetToggleEffectsRecursive(Top, RemoveFromSel, AddToSel);
        }
        else
        {
            ArrayT<MapElementT*> Ignore;

            // Don't toggle piece-wise, but rather put all parts of Top into the same new selection state as Top itself.
            if (Top->GetRepres()->IsSelected())
            {
                GetToggleEffectsRecursive(Top, RemoveFromSel, Ignore);
            }
            else
            {
                GetToggleEffectsRecursive(Top, Ignore, AddToSel);
            }
        }
    }
    else
    {
        // Insert this element into one of the lists, but only if it isn't mentioned there already.
        if (RemoveFromSel.Find(this) == -1 && AddToSel.Find(this) == -1)
        {
            if (IsSelected()) RemoveFromSel.PushBack(this);
                         else AddToSel.PushBack(this);
        }
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
