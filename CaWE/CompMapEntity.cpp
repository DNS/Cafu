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
#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "MapPrimitive.hpp"


using namespace MapEditor;


CompMapEntityT::CompMapEntityT(MapDocumentT& MapDoc)
    : ComponentBaseT(),
      m_MapDoc(MapDoc),
      m_Class(NULL),
      m_Properties(),
      m_Repres(NULL),
      m_Primitives()
{
    m_Repres = new MapEntRepresT(this);
}


CompMapEntityT::CompMapEntityT(const CompMapEntityT& Comp)
    : ComponentBaseT(Comp),
      m_MapDoc(Comp.m_MapDoc),
      m_Class(Comp.m_Class),
      m_Properties(Comp.m_Properties),
      m_Repres(NULL),
      m_Primitives()
{
    // Note that the m_Primitives are intentionally *not* copied here!
    m_Repres = new MapEntRepresT(this);
}


CompMapEntityT::~CompMapEntityT()
{
    // Delete all our primitives.
    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        delete m_Primitives[PrimNr];

    delete m_Repres;
    m_Repres = NULL;
}


CompMapEntityT* CompMapEntityT::Clone() const
{
    return new CompMapEntityT(*this);
}


void CompMapEntityT::Render() const
{
}


bool CompMapEntityT::IsWorld() const
{
    return m_MapDoc.GetEntities()[0] == this;
}


void CompMapEntityT::SetClass(const EntityClassT* NewClass)
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
}


EntPropertyT* CompMapEntityT::FindProperty(const wxString& Key, int* Index, bool Create)
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


const EntPropertyT* CompMapEntityT::FindProperty(const wxString& Key, int* Index) const
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


int CompMapEntityT::FindPropertyIndex(const wxString& Key) const
{
    for (unsigned long PropNr=0; PropNr<m_Properties.Size(); PropNr++)
        if (m_Properties[PropNr].Key==Key)
            return PropNr;

    return -1;
}


void CompMapEntityT::RemoveProperty(const wxString& Key)
{
    for (unsigned long PropNr = 0; PropNr < m_Properties.Size(); PropNr++)
        if (m_Properties[PropNr].Key == Key)
        {
            m_Properties.RemoveAtAndKeepOrder(PropNr);
            return;
        }
}


std::string CompMapEntityT::GetAndRemove(const wxString& Key, const char* Default)
{
    const EntPropertyT* Prop = FindProperty(Key);

    if (!Prop) return Default;

    const wxString Value = Prop->Value;

    RemoveProperty(Key);

    return Value.ToStdString();
}


void CompMapEntityT::CopyPrimitives(const CompMapEntityT& MapEnt)
{
    for (unsigned long PrimNr = 0; PrimNr < MapEnt.m_Primitives.Size(); PrimNr++)
    {
        m_Primitives.PushBack(MapEnt.m_Primitives[PrimNr]->Clone());
        m_Primitives[PrimNr]->SetParent(this);
    }
}


void CompMapEntityT::AddPrim(MapPrimitiveT* Prim)
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


void CompMapEntityT::RemovePrim(MapPrimitiveT* Prim)
{
    const int Index=m_Primitives.Find(Prim);

    wxASSERT(Prim->GetParent() == this);
    Prim->SetParent(NULL);

    wxASSERT(Index>=0);
    if (Index==-1) return;

    // Keeping the order helps when map files are diff'ed or manually compared.
    m_Primitives.RemoveAtAndKeepOrder(Index);
}


BoundingBox3fT CompMapEntityT::GetElemsBB() const
{
    BoundingBox3fT BB = m_Repres->GetBB();

    for (unsigned int PrimNr = 0; PrimNr < m_Primitives.Size(); PrimNr++)
        BB += m_Primitives[PrimNr]->GetBB();

    return BB;
}
