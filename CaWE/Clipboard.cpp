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

#include "Clipboard.hpp"
#include "CompMapEntity.hpp"
#include "MapEntRepres.hpp"
#include "MapPrimitive.hpp"


using namespace MapEditor;


// We cannot define this constructor inline in the "Clipboard.hpp" header file,
// because the (Visual C++) compiler would then expect us to #include the "GameSys/Entity.hpp" header
// file in quasi every .cpp file in the application (or in "Clipboard.hpp").
// I don't know exactly why this is so, but obviously the inline definition triggers the instantiation
// of the IntrusivePtrT<cf::GameSys::EntityT>, which in the header file are only forward-declared.
ClipboardT::ClipboardT()
{
}


ClipboardT::~ClipboardT()
{
    Clear();
}


void ClipboardT::CopyFrom(const ArrayT<MapElementT*>& Elems)
{
    Clear();

    ArrayT< IntrusivePtrT<CompMapEntityT> > SourceEnts;

    // First pass: Consider the MapEntRepresT instances.
    for (unsigned long ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
    {
        MapEntRepresT* Repres = dynamic_cast<MapEntRepresT*>(Elems[ElemNr]);

        if (Repres)
        {
            SourceEnts.PushBack(Repres->GetParent());

            // TODO: The new instance is referring to the original MapDoc and its entity classes!!!!!

            // Note that we don't want the primitives of the source entity copied!
            /*
             *
             *       TODO -- Must e.g. set a flag in the CompMapEntity to no copy the primitives!
             *
             */
            m_Entities.PushBack(Repres->GetParent()->GetEntity()->Clone());
        }
    }

    // Second pass: Consider the MapPrimitiveT instances.
    for (unsigned long ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
    {
        MapPrimitiveT* Prim = dynamic_cast<MapPrimitiveT*>(Elems[ElemNr]);

        if (Prim)
        {
            const int EntNr = SourceEnts.Find(Prim->GetParent());

            if (EntNr >= 0)
            {
                GetMapEnt(m_Entities[EntNr])->AddPrim(Prim->Clone());
            }
            else
            {
                m_Primitives.PushBack(Prim->Clone());
            }
        }
    }
}


void ClipboardT::Clear()
{
    for (unsigned long EntNr = 0; EntNr < m_Entities.Size(); EntNr++)
    {
        m_Entities[EntNr] = NULL;
    }

    m_Entities.Overwrite();

    for (unsigned long PrimNr = 0; PrimNr < m_Primitives.Size(); PrimNr++)
    {
        delete m_Primitives[PrimNr];
        m_Primitives[PrimNr] = NULL;
    }

    m_Primitives.Overwrite();
}
