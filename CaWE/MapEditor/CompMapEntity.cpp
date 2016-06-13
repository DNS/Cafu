/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompMapEntity.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "MapPrimitive.hpp"


using namespace MapEditor;


CompMapEntityT::CompMapEntityT(MapDocumentT& MapDoc)
    : ComponentBaseT(),
      m_MapDoc(MapDoc),
      m_Properties(),
      m_Repres(NULL),
      m_Primitives()
{
    m_Repres = new MapEntRepresT(this);
}


CompMapEntityT::CompMapEntityT(const CompMapEntityT& Comp)
    : ComponentBaseT(Comp),
      m_MapDoc(Comp.m_MapDoc),
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
    return m_MapDoc.GetRootMapEntity() == this;
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


void CompMapEntityT::RemoveProperty(const wxString& Key)
{
    for (unsigned long PropNr = 0; PropNr < m_Properties.Size(); PropNr++)
        if (m_Properties[PropNr].Key == Key)
        {
            m_Properties.RemoveAtAndKeepOrder(PropNr);
            return;
        }
}


std::string CompMapEntityT::GetProperty(const wxString& Key, const char* Default) const
{
    const EntPropertyT* Prop = FindProperty(Key);

    if (!Prop) return Default;

    return Prop->Value.ToStdString();
}


std::string CompMapEntityT::GetAndRemove(const wxString& Key, const char* Default)
{
    const EntPropertyT* Prop = FindProperty(Key);

    if (!Prop) return Default;

    const wxString Value = Prop->Value;

    RemoveProperty(Key);

    return Value.ToStdString();
}


void CompMapEntityT::CopyPrimitives(const CompMapEntityT& MapEnt, bool Recursive)
{
    for (unsigned long PrimNr = 0; PrimNr < MapEnt.m_Primitives.Size(); PrimNr++)
    {
        m_Primitives.PushBack(MapEnt.m_Primitives[PrimNr]->Clone());
        m_Primitives[PrimNr]->SetParent(this);
    }

    if (!Recursive) return;

    const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& ThisChildren  = GetEntity()->GetChildren();
    const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& OtherChildren = MapEnt.GetEntity()->GetChildren();

    wxASSERT(ThisChildren.Size() == OtherChildren.Size());

    for (unsigned int EntNr = 0; EntNr < ThisChildren.Size() && EntNr < OtherChildren.Size(); EntNr++)
        GetMapEnt(ThisChildren[EntNr])->CopyPrimitives(*GetMapEnt(OtherChildren[EntNr]), true);
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


ArrayT<MapElementT*> CompMapEntityT::GetAllMapElements() const
{
    ArrayT<MapElementT*> AllElems;
    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > AllEnts;

    GetEntity()->GetAll(AllEnts);

    for (unsigned int EntNr = 0; EntNr < AllEnts.Size(); EntNr++)
    {
        IntrusivePtrT<CompMapEntityT> MapEnt = GetMapEnt(AllEnts[EntNr]);

        AllElems.PushBack(MapEnt->GetRepres());

        for (unsigned int PrimNr = 0; PrimNr < MapEnt->GetPrimitives().Size(); PrimNr++)
            AllElems.PushBack(MapEnt->GetPrimitives()[PrimNr]);
    }

    return AllElems;
}


BoundingBox3fT CompMapEntityT::GetElemsBB() const
{
    BoundingBox3fT BB = m_Repres->GetBB();

    for (unsigned int PrimNr = 0; PrimNr < m_Primitives.Size(); PrimNr++)
        BB += m_Primitives[PrimNr]->GetBB();

    return BB;
}
