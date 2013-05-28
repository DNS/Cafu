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
#include "MapEntityBase.hpp"
#include "GameSys/AllComponents.hpp"


using namespace MapEditor;


CompMapEntityT::CompMapEntityT(MapDocumentT& MapDoc)
    : ComponentBaseT(),
      m_MapEntity(new MapEntityBaseT(MapDoc, this))
{
}


CompMapEntityT::CompMapEntityT(const CompMapEntityT& Comp)
    : ComponentBaseT(Comp),
      m_MapEntity(new MapEntityBaseT(*Comp.m_MapEntity, this))
{
}


CompMapEntityT::~CompMapEntityT()
{
    delete m_MapEntity;
}


CompMapEntityT* CompMapEntityT::Clone() const
{
    return new CompMapEntityT(*this);
}


void CompMapEntityT::Render() const
{
}


void CompMapEntityT::Load_cmap(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr, unsigned int& cmapVersion)
{
    m_MapEntity->Load_cmap(TP, MapDoc, ProgressDialog, EntityNr, cmapVersion);
}


void CompMapEntityT::Load_HL1_map(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr)
{
    m_MapEntity->Load_HL1_map(TP, MapDoc, ProgressDialog, EntityNr);
}


void CompMapEntityT::Load_HL2_vmf(TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr)
{
    m_MapEntity->Load_HL2_vmf(TP, MapDoc, ProgressDialog, EntityNr);
}


void CompMapEntityT::Load_D3_map (TextParserT& TP, MapDocumentT& MapDoc, wxProgressDialog* ProgressDialog, unsigned long EntityNr)
{
    m_MapEntity->Load_D3_map(TP, MapDoc, ProgressDialog, EntityNr);
}


void CompMapEntityT::Save_cmap(const MapDocumentT& MapDoc, std::ostream& OutFile, unsigned long EntityNr, const BoundingBox3fT* Intersecting) const
{
    m_MapEntity->Save_cmap(MapDoc, OutFile, EntityNr, Intersecting);
}


bool CompMapEntityT::IsWorld() const
{
    return m_MapEntity->IsWorld();
}


void CompMapEntityT::SetClass(const EntityClassT* NewClass)
{
    m_MapEntity->SetClass(NewClass);
}


Vector3fT CompMapEntityT::GetOrigin() const
{
    return m_MapEntity->GetOrigin();
}


void CompMapEntityT::SetOrigin(const Vector3fT& Origin)
{
    m_MapEntity->SetOrigin(Origin);
}


EntPropertyT* CompMapEntityT::FindProperty(const wxString& Key, int* Index, bool Create)
{
    return m_MapEntity->FindProperty(Key, Index, Create);
}


const EntPropertyT* CompMapEntityT::FindProperty(const wxString& Key, int* Index) const
{
    return m_MapEntity->FindProperty(Key, Index);
}


int CompMapEntityT::FindPropertyIndex(const wxString& Key) const
{
    return m_MapEntity->FindPropertyIndex(Key);
}


cf::math::AnglesfT CompMapEntityT::GetAngles() const
{
    return m_MapEntity->GetAngles();
}


void CompMapEntityT::SetAngles(const cf::math::AnglesfT& Angles)
{
    m_MapEntity->SetAngles(Angles);
}


void CompMapEntityT::AddPrim(MapPrimitiveT* Prim)
{
    m_MapEntity->AddPrim(Prim);
}


void CompMapEntityT::RemovePrim(MapPrimitiveT* Prim)
{
    m_MapEntity->RemovePrim(Prim);
}


BoundingBox3fT CompMapEntityT::GetElemsBB() const
{
    return m_MapEntity->GetElemsBB();
}


ArrayT<EntPropertyT> CompMapEntityT::CheckUniqueValues(bool Repair)
{
    return m_MapEntity->CheckUniqueValues(Repair);
}
