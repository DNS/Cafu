/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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


namespace
{
    bool CanToggleAll(const ArrayT<MapElementT*>& AllElems)
    {
        for (unsigned long ElemNr = 0; ElemNr < AllElems.Size(); ElemNr++)
            if (!AllElems[ElemNr]->IsVisible() || !AllElems[ElemNr]->CanSelect())
                return false;

        return true;
    }
}


void MapElementT::GetToggleEffects(ArrayT<MapElementT*>& RemoveFromSel, ArrayT<MapElementT*>& AddToSel, bool AutoGroupEntities)
{
    IntrusivePtrT<CompMapEntityT> MapEnt = GetParent();

    // If this element is a member of a group, put all members of the group into the appropriate lists.
    bool AllInThisEntity = true;

    if (m_Group && m_Group->SelectAsGroup)
    {
        const ArrayT<MapElementT*> GroupMembers = m_Group->GetMembers();

        for (unsigned long MemberNr = 0; MemberNr < GroupMembers.Size(); MemberNr++)
        {
            MapElementT* Member = GroupMembers[MemberNr];

            if (!MapEnt->GetEntity()->Has(Member->GetParent()->GetEntity()))
                AllInThisEntity = false;

            // Insert Member into one of the lists, but only if it isn't mentioned there already.
            if (RemoveFromSel.Find(Member) == -1 && AddToSel.Find(Member) == -1)
            {
                if (Member->IsSelected()) RemoveFromSel.PushBack(Member);
                                     else AddToSel.PushBack(Member);
            }
        }
    }

    // If
    //   - auto-grouping is enabled,
    //   - this element belongs to a non-world entity,
    //   - the previous group-selection did not toggle elements in other entities and
    //   - the entity does not contain elements that are (in groups that are) invisible or locked,
    // put all elements of the entity into the appropriate lists.
    //
    // Note that we don't "recurse": If the entity contains elements that are in another group
    // (other than m_Group) with group-selection enabled, the selection is not extended to cover
    // the other group as well, as this would probably be unexpected and confusing for the user.
    if (AutoGroupEntities && !MapEnt->IsWorld() && AllInThisEntity)
    {
        const ArrayT<MapElementT*> AllElems = MapEnt->GetAllMapElements();

        if (CanToggleAll(AllElems))
        {
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
