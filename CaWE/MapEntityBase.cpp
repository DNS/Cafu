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

#include "MapEntityBase.hpp"
#include "EntityClass.hpp"
#include "EntityClassVar.hpp"
#include "LuaAux.hpp"
#include "MapPrimitive.hpp"

#include "wx/wx.h"
#include "wx/sstream.h"
#include "wx/txtstrm.h"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapEntityBaseT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapEntityBaseT::TypeInfo(GetMapElemTIM(), "MapEntityBaseT", "MapElementT", MapEntityBaseT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapEntityBaseT::MapEntityBaseT(const wxColour& Color)
    : MapElementT(Color),
      m_Class(NULL),
      m_Properties(),
      m_Primitives()
{
}


MapEntityBaseT::MapEntityBaseT(const MapEntityBaseT& Ent)
    : MapElementT(Ent),
      m_Class(Ent.m_Class),
      m_Properties(Ent.m_Properties),
      m_Primitives()
{
    // Deep-copy all primitives of Ent.
    for (unsigned long PrimNr=0; PrimNr<Ent.m_Primitives.Size(); PrimNr++)
    {
        m_Primitives.PushBack(Ent.m_Primitives[PrimNr]->Clone());
        m_Primitives[PrimNr]->SetParent(this);
    }
}


MapEntityBaseT::~MapEntityBaseT()
{
    // Delete all our primitives.
    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        delete m_Primitives[PrimNr];
}


// MapEntityBaseT* MapEntityBaseT::Clone() const
// {
//     // Cannot instantiate an abstract class - we had to provide implementations for all pure virtual
//     // methods (like e.g. GetBB()) in order to be able to explicitly create MapEntityBaseT instances.
//     return new MapEntityBaseT(*this);
// }


void MapEntityBaseT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapElementT::Assign(Elem);

    const MapEntityBaseT* Entity=dynamic_cast<const MapEntityBaseT*>(Elem);
    wxASSERT(Entity!=NULL);
    if (Entity==NULL) return;

    m_Class     =Entity->m_Class;
    m_Properties=Entity->m_Properties;

    // Delete all our previous primitives.
    for (unsigned long PrimNr=0; PrimNr<m_Primitives.Size(); PrimNr++)
        delete m_Primitives[PrimNr];

    m_Primitives.Overwrite();

    // Deep-copy all primitives of Entity.
    for (unsigned long PrimNr=0; PrimNr<Entity->m_Primitives.Size(); PrimNr++)
    {
        m_Primitives.PushBack(Entity->m_Primitives[PrimNr]->Clone());
        m_Primitives[PrimNr]->SetParent(this);
    }
}


void MapEntityBaseT::SetClass(const EntityClassT* NewClass)
{
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
