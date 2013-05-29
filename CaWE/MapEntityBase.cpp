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

#include "MapEntityBase.hpp"
#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "MapPrimitive.hpp"

#include "wx/wx.h"
#include "wx/sstream.h"
#include "wx/txtstrm.h"


MapEntityBaseT::MapEntityBaseT(MapDocumentT& MapDoc, MapEditor::CompMapEntityT* CompMapEnt)
    : m_MapDoc(MapDoc),
      m_CompMapEnt(CompMapEnt),
      m_Class(NULL),
      m_Origin(),
      m_Properties(),
      m_Repres(NULL),
      m_Primitives()
{
    m_Repres = new MapEntRepresT(this);
}


MapEntityBaseT::MapEntityBaseT(const MapEntityBaseT& Ent, MapEditor::CompMapEntityT* CompMapEnt, bool CopyPrims)
    : m_MapDoc(Ent.m_MapDoc),
      m_CompMapEnt(CompMapEnt),
      m_Class(Ent.m_Class),
      m_Origin(Ent.m_Origin),
      m_Properties(Ent.m_Properties),
      m_Repres(NULL),
      m_Primitives()
{
    // Deep-copy all primitives of Ent.
    if (CopyPrims)
    {
        for (unsigned long PrimNr = 0; PrimNr < Ent.m_Primitives.Size(); PrimNr++)
        {
            m_Primitives.PushBack(Ent.m_Primitives[PrimNr]->Clone());
            m_Primitives[PrimNr]->SetParent(this);
        }
    }

    m_Repres = new MapEntRepresT(this);
}


MapEntityBaseT::~MapEntityBaseT()
{
    // Delete all our primitives.
    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        delete m_Primitives[PrimNr];

    delete m_Repres;
    m_Repres = NULL;
}


bool MapEntityBaseT::IsWorld() const
{
    return m_MapDoc.GetEntities()[0] == this;
}


void MapEntityBaseT::SetClass(const EntityClassT* NewClass)
{
    if (m_Class == NewClass) return;

    m_Class=NewClass;

    // Instantiate the variables declared in the entity class in the conrete entity.
    for (unsigned long VarNr=0; VarNr<m_Class->GetVariables().Size(); VarNr++)
    {
        const EntClassVarT* ClassVar=m_Class->GetVariables()[VarNr];

        // When no instance with the same name has been instantiated in this entity yet,
        // create and add a new instance now.
        if (FindProperty(ClassVar->GetName())==NULL)
            m_Properties.PushBack(ClassVar->GetInstance());
    }


    // Now that we have a new class, update the entity representation in the map.
    m_Repres->Update();
}


Vector3fT MapEntityBaseT::GetOrigin() const
{
    if (!m_Class->IsSolidClass()) return m_Origin;

    // This is very similar to GetBB().GetCenter(), but without accounting for the helpers.
    // The helpers GetBB() methods call m_ParentEntity->GetOrigin(), possibly creating an infinite recursion.
    BoundingBox3fT BB;

    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        BB+=m_Primitives[PrimNr]->GetBB();

    return BB.IsInited() ? BB.GetCenter() : m_Origin;
}


void MapEntityBaseT::SetOrigin(const Vector3fT& Origin)
{
    m_Origin=Origin;
}


EntPropertyT* MapEntityBaseT::FindProperty(const wxString& Key, int* Index, bool Create)
{
    for (unsigned long PropNr=0; PropNr<m_Properties.Size(); PropNr++)
    {
        if (m_Properties[PropNr].Key==Key)
        {
            if (Index!=NULL) *Index=PropNr;

            return &m_Properties[PropNr];
        }
    }

    if (Create)
    {
        m_Properties.PushBack(EntPropertyT(Key, ""));

        if (Index!=NULL) *Index=m_Properties.Size()-1;

        return &m_Properties[m_Properties.Size()-1];
    }

    if (Index!=NULL) *Index=-1;

    return NULL;
}


const EntPropertyT* MapEntityBaseT::FindProperty(const wxString& Key, int* Index) const
{
    for (unsigned long PropNr=0; PropNr<m_Properties.Size(); PropNr++)
    {
        if (m_Properties[PropNr].Key==Key)
        {
            if (Index!=NULL) *Index=PropNr;

            return &m_Properties[PropNr];
        }
    }

    if (Index!=NULL) *Index=-1;

    return NULL;
}


int MapEntityBaseT::FindPropertyIndex(const wxString& Key) const
{
    for (unsigned long PropNr=0; PropNr<m_Properties.Size(); PropNr++)
        if (m_Properties[PropNr].Key==Key)
            return PropNr;

    return -1;
}


cf::math::AnglesfT MapEntityBaseT::GetAngles() const
{
    cf::math::AnglesfT  Angles;
    const EntPropertyT* AnglesProp=FindProperty("angles");

    if (AnglesProp!=NULL)
    {
        wxStringInputStream sis(AnglesProp->Value);
        wxTextInputStream   tis(sis);

        tis >> Angles[PITCH] >> Angles[YAW] >> Angles[ROLL];
    }

    return Angles;
}


void MapEntityBaseT::SetAngles(const cf::math::AnglesfT& Angles)
{
    FindProperty("angles", NULL, true)->Value=wxString::Format("%g %g %g", Angles[PITCH], Angles[YAW], Angles[ROLL]);
}


void MapEntityBaseT::AddPrim(MapPrimitiveT* Prim)
{
    Prim->SetParent(this);

    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        if (m_Primitives[PrimNr]==Prim)
        {
            wxASSERT(false);
            return;
        }

    m_Primitives.PushBack(Prim);
}


void MapEntityBaseT::RemovePrim(MapPrimitiveT* Prim)
{
    const int Index=m_Primitives.Find(Prim);

    wxASSERT(Prim->GetParent()==this);
    Prim->SetParent(NULL);

    wxASSERT(Index>=0);
    if (Index==-1) return;

    // Keeping the order helps when map files are diff'ed or manually compared.
    m_Primitives.RemoveAtAndKeepOrder(Index);
}


BoundingBox3fT MapEntityBaseT::GetElemsBB() const
{
    BoundingBox3fT BB = m_Repres->GetBB();

    for (unsigned int PrimNr = 0; PrimNr < m_Primitives.Size(); PrimNr++)
        BB += m_Primitives[PrimNr]->GetBB();

    return BB;
}


ArrayT<EntPropertyT> MapEntityBaseT::CheckUniqueValues(bool Repair)
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
                for (unsigned long EntNr=1/*skip world*/; EntNr<m_MapDoc.GetEntities().Size(); EntNr++)
                {
                    const MapEntityBaseT* Ent    =m_MapDoc.GetEntities()[EntNr];
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

                for (EntNr=1/*skip world*/; EntNr<m_MapDoc.GetEntities().Size(); EntNr++)
                {
                    const MapEntityBaseT* Ent    =m_MapDoc.GetEntities()[EntNr];
                    const EntPropertyT*   EntProp=Ent->FindProperty("name");

                    if (Ent!=this && EntProp && EntProp->Value==UniqueValue)
                    {
                        FoundEnt=Ent;
                        break;
                    }
                }

                if (EntNr>=m_MapDoc.GetEntities().Size())
                {
                    FindProperty("name", NULL, true)->Value=UniqueValue;

                    ArrayT<MapEntityBaseT*> MapElements;
                    MapElements.PushBack(this);

                    m_MapDoc.UpdateAllObservers_Modified(MapElements, MEMD_ENTITY_PROPERTY_MODIFIED, "name");
                    break;
                }
            }
        }
    }

    return FoundVars;
}
