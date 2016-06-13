/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "MakeHollow.hpp"
#include "Delete.hpp"
#include "Select.hpp"

#include "../CompMapEntity.hpp"
#include "../Group.hpp"
#include "../MapBrush.hpp"
#include "../MapDocument.hpp"


using namespace MapEditor;


CommandMakeHollowT::CommandMakeHollowT(MapDocumentT& MapDoc, const float WallWidth, const ArrayT<MapElementT*>& Elems)
    : m_MapDoc(MapDoc),
      m_CmdDelete(NULL),
      m_CmdSelect(NULL)
{
    // Make sure that WallWidth isn't too small for reasonably hollowing the brushes.
    if (WallWidth>-1.0f && WallWidth<1.0f) return;

    // Isolate from Elems the map elements that are eligible for hollowing: the brushes.
    for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
    {
        MapBrushT* Brush=dynamic_cast<MapBrushT*>(Elems[ElemNr]);

        if (Brush)
        {
            m_Brushes.PushBack(Brush);
            m_Hollows.PushBackEmpty();
        }
    }

    // Hollow each brush by subtracting from it a scaled copy of itself.
    for (unsigned long BrushNr=0; BrushNr<m_Brushes.Size(); BrushNr++)
    {
        const MapBrushT* Brush=m_Brushes[BrushNr];
        MapBrushT        ScaledCopy(*Brush);

        // Transform ScaledCopy as required to reduce its size by WallWidth.
        const BoundingBox3fT BB=Brush->GetBB();
        bool                 TooSmall=false;
        Vector3fT            Scale;

        for (int i=0; i<3; i++)
        {
            const float Half=(BB.Max[i]-BB.Min[i])/2.0f;

            Scale[i]=std::max((Half-WallWidth)/Half, 0.0f);
            if (WallWidth+1.0f > Half) TooSmall=true;
        }

        // If the scaled copy will be less than 2 units wide along some axis, don't hollow the current brush.
        if (TooSmall) continue;

        try
        {
            ScaledCopy.TrafoScale(BB.GetCenter(), Scale, false /*LockTexCoords*/);
        }
        // If scaling ScaledCopy failed (e.g. because Scale was at or near 0), don't hollow the current brush.
        catch (const DivisionByZeroE&) { continue; }

        // Setup the operands A and B. Negative wall widths reverse the subtraction.
        const MapBrushT* A=WallWidth>0 ? Brush : &ScaledCopy;   // The larger brush.
        const MapBrushT* B=WallWidth>0 ? &ScaledCopy : Brush;   // The smaller brush.

        // Perform the subtraction.
        ArrayT<MapBrushT*> Result;

        if (A->Subtract(B, Result))
        {
            // Note that Result can contain any number of brushes: zero, one, or many.
            m_Hollows[BrushNr]=Result;
        }
        else
        {
            // No subtraction, delete anything in Result.
            for (unsigned long ResNr=0; ResNr<Result.Size(); ResNr++)
                delete Result[ResNr];
        }
    }

    // Drop any brush/hollow pairs where no proper hollow could be created for the brush.
    for (unsigned long BrushNr=0; BrushNr<m_Brushes.Size(); BrushNr++)
    {
        if (m_Hollows[BrushNr].Size()==0)
        {
            m_Brushes.RemoveAt(BrushNr);
            m_Hollows.RemoveAt(BrushNr);
            BrushNr--;
        }
    }

    // Assign the proper groups to the new hollows.
    for (unsigned long BrushNr=0; BrushNr<m_Brushes.Size(); BrushNr++)
    {
        const MapBrushT*          Brush = m_Brushes[BrushNr];
        const ArrayT<MapBrushT*>& Walls = m_Hollows[BrushNr];

        if (Brush->GetGroup())
        {
            // If the original brush was in a group, make sure that all the hollow walls are in the same group.
            for (unsigned long WallNr = 0; WallNr < Walls.Size(); WallNr++)
                Walls[WallNr]->SetGroup(Brush->GetGroup());
        }
        else
        {
            GroupT* Group = new GroupT(m_MapDoc, "hollowed brush");
            Group->SelectAsGroup = true;

            for (unsigned long WallNr = 0; WallNr < Walls.Size(); WallNr++)
                Walls[WallNr]->SetGroup(Group);

            m_NewGroups.PushBack(Group);
        }
    }
}


CommandMakeHollowT::~CommandMakeHollowT()
{
    if (!m_Done)
    {
        for (unsigned long HollowNr=0; HollowNr<m_Hollows.Size(); HollowNr++)
        {
            const ArrayT<MapBrushT*>& Walls=m_Hollows[HollowNr];

            for (unsigned long WallNr=0; WallNr<Walls.Size(); WallNr++)
                delete Walls[WallNr];
        }

        for (unsigned long GroupNr=0; GroupNr<m_NewGroups.Size(); GroupNr++)
            delete m_NewGroups[GroupNr];
    }

    delete m_CmdDelete;
    delete m_CmdSelect;
}


bool CommandMakeHollowT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    wxASSERT(m_Brushes.Size()==m_Hollows.Size());
    if (m_Brushes.Size()==0) return false;

    // Add the new groups (if any),
    // and notify all observers that our groups inventory changed.
    if (m_NewGroups.Size()>0)
    {
        m_MapDoc.GetGroups().PushBack(m_NewGroups);
        m_MapDoc.UpdateAllObservers_GroupsChanged();
    }

    // Replace the brushes with the (walls of the) hollows.
    ArrayT<MapElementT*>   BrushElems;
    ArrayT<MapPrimitiveT*> WallElems;

    for (unsigned long BrushNr=0; BrushNr<m_Brushes.Size(); BrushNr++)
    {
        MapBrushT*                Brush=m_Brushes[BrushNr];
        const ArrayT<MapBrushT*>& Walls=m_Hollows[BrushNr];

        BrushElems.PushBack(Brush);

        for (unsigned long WallNr=0; WallNr<Walls.Size(); WallNr++)
        {
            m_MapDoc.Insert(Walls[WallNr], Brush->GetParent());
            WallElems.PushBack(Walls[WallNr]);
        }
    }

    m_MapDoc.UpdateAllObservers_Created(WallElems);

    // Delete the original brushes.
    if (!m_CmdDelete) m_CmdDelete=new CommandDeleteT(m_MapDoc, BrushElems);
    m_CmdDelete->Do();

    // Set the (walls of the) newly created hollows as the new selection.
    if (!m_CmdSelect) m_CmdSelect=CommandSelectT::Set(&m_MapDoc, WallElems);
    m_CmdSelect->Do();

    m_Done=true;
    return true;
}


void CommandMakeHollowT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    m_CmdSelect->Undo();
    m_CmdDelete->Undo();

    // Replace the (walls of the) hollows with the brushes again.
    ArrayT<MapPrimitiveT*> WallElems;

    for (unsigned long HollowNr=0; HollowNr<m_Hollows.Size(); HollowNr++)
    {
        const ArrayT<MapBrushT*>& Walls=m_Hollows[HollowNr];

        for (unsigned long WallNr=0; WallNr<Walls.Size(); WallNr++)
        {
            m_MapDoc.Remove(Walls[WallNr]);
            WallElems.PushBack(Walls[WallNr]);
        }
    }

    m_MapDoc.UpdateAllObservers_Deleted(WallElems);

    // Remove the new groups again (if any),
    // and notify all observers that our groups inventory changed.
    if (m_NewGroups.Size()>0)
    {
        m_MapDoc.GetGroups().DeleteBack(m_NewGroups.Size());
        m_MapDoc.UpdateAllObservers_GroupsChanged();
    }

    m_Done=false;
}


wxString CommandMakeHollowT::GetName() const
{
    return "Make hollow";
}
