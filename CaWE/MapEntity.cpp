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
    : MapEntityBaseT(MapDoc, Options.colors.Entity)
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


void MapEntityT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapEntityBaseT::Assign(Elem);

    const MapEntityT* Ent=dynamic_cast<const MapEntityT*>(Elem);
    wxASSERT(Ent!=NULL);
    if (Ent==NULL) return;
}


wxColour MapEntityT::GetColor(bool ConsiderGroup) const
{
    if (m_Group && ConsiderGroup)
        return m_Group->Color;

    return m_Class->GetColor();
}


wxString MapEntityT::GetDescription() const
{
    const EntPropertyT* NameProp=FindProperty("name");

    return m_Class->GetName()+" entity"+(NameProp ? " ("+NameProp->Value+")" : "");
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


bool MapEntityT::TraceRay(const Vector3fT& RayOrigin, const Vector3fT& RayDir, float& Fraction, unsigned long& FaceNr) const
{
    // Entities are hit indirectly via their MapEntRepresT,
    // or via their primitives (brushes, Bezier patches, terrains, etc.).
    return false;
}


bool MapEntityT::TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const
{
    return false;
}


ArrayT<EntPropertyT> MapEntityT::CheckUniqueValues(MapDocumentT& MapDoc, bool Repair)
{
    // All nonunique properties that are found (and repaired if Repair is true).
    ArrayT<EntPropertyT> FoundVars;

    // Only handle entities that have a class.
    if (m_Class==NULL) return FoundVars;

    const ArrayT<const EntClassVarT*>& ClassVars=m_Class->GetVariables();

    // For all class vars (only class vars can be unique).
    for (unsigned long i=0; i<ClassVars.Size(); i++)
    {
        // Check if class var is unique.
        // FIXME: At the moment only the "name" variable is supported, as this is the only unique variable at this time.
        if (ClassVars[i]->IsUnique() && ClassVars[i]->GetName()=="name")
        {
            // Remember current value.
            EntPropertyT*         FoundProp=FindProperty("name");
            const MapEntityBaseT* FoundEnt =NULL;

            // If value is set.
            if (FoundProp!=NULL)
            {
                for (unsigned long EntNr=1/*skip world*/; EntNr<MapDoc.GetEntities().Size(); EntNr++)
                {
                    const MapEntityBaseT* Ent    =MapDoc.GetEntities()[EntNr];
                    const EntPropertyT*   EntProp=Ent->FindProperty("name");

                    if (Ent!=this && EntProp && EntProp->Value==FoundProp->Value)
                    {
                        FoundEnt=Ent;
                        break;
                    }
                }
            }

            // Found a faulty value.
            if (FoundEnt!=NULL || FoundProp==NULL)
            {
                // Set property value to default.
                wxString ValueTmp=ClassVars[i]->GetDefault();

                // Overwrite default value by concrete value if property is set.
                if (FoundProp!=NULL) ValueTmp=FoundProp->Value;

                FoundVars.PushBack(EntPropertyT("name", ValueTmp));
            }

            // User doesn't want the faulty value to be repaired or there is nothing to repair.
            if (!Repair || (FoundEnt==NULL && FoundProp!=NULL)) continue;

            // Repair faulty value.
            for (unsigned long Count=1; true; Count++)
            {
                const wxString UniqueValue=CheckLuaIdentifier(m_Class->GetName())+wxString::Format("_%03lu", Count);
                unsigned long  EntNr;

                for (EntNr=1/*skip world*/; EntNr<MapDoc.GetEntities().Size(); EntNr++)
                {
                    const MapEntityBaseT* Ent    =MapDoc.GetEntities()[EntNr];
                    const EntPropertyT*   EntProp=Ent->FindProperty("name");

                    if (Ent!=this && EntProp && EntProp->Value==UniqueValue)
                    {
                        FoundEnt=Ent;
                        break;
                    }
                }

                if (EntNr>=MapDoc.GetEntities().Size())
                {
                    FindProperty("name", NULL, true)->Value=UniqueValue;

                    ArrayT<MapEntityBaseT*> MapElements;
                    MapElements.PushBack(this);

                    MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, "name");
                    break;
                }
            }
        }
    }

    return FoundVars;
}
