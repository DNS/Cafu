/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Clip.hpp"
#include "Select.hpp"
#include "Delete.hpp"

#include "../CompMapEntity.hpp"
#include "../MapDocument.hpp"


CommandClipT::CommandClipT(MapDocumentT& MapDoc_, const ArrayT<ClipResultT*>& ClipResults_)
    : MapDoc(MapDoc_),
      ClipResults(ClipResults_),
      CommandSelect(NULL),
      CommandDelete(NULL)
{
}


CommandClipT::~CommandClipT()
{
    for (unsigned long ClipNr=0; ClipNr<ClipResults.Size(); ClipNr++)
    {
        // If command has been performed and the front/back brushes are in the world,
        // prevent the ClipResultT from deleting them by setting their pointers to NULL.
        // The deletion of the originating brushes (the workpieces) is handled by CommandDelete.
        if (m_Done)
        {
            ClipResults[ClipNr]->Front=NULL;
            ClipResults[ClipNr]->Back =NULL;
        }

        delete ClipResults[ClipNr];
    }

    delete CommandSelect;
    delete CommandDelete;
}


bool CommandClipT::Do()
{
    wxASSERT(!m_Done);

    if (m_Done) return false;

    ArrayT<MapPrimitiveT*> InsertedBrushes;   // The list of brushes that are newly inserted into the world.
    ArrayT<MapElementT*>   RemovedBrushes;    // The list of originator brushes that are removed from the world.

    // Replace the originating workpieces with their clipped front/back brushes.
    for (unsigned long ClipNr=0; ClipNr<ClipResults.Size(); ClipNr++)
    {
        ClipResultT* ClipResult=ClipResults[ClipNr];

        if (!ClipResult) continue;

        MapBrushT* Workpiece=ClipResult->Workpiece;
        MapBrushT* Front    =ClipResult->Front;
        MapBrushT* Back     =ClipResult->Back;

        if (Front)
        {
            MapDoc.Insert(Front, Workpiece->GetParent());
            InsertedBrushes.PushBack(Front);
        }

        if (Back)
        {
            MapDoc.Insert(Back, Workpiece->GetParent());
            InsertedBrushes.PushBack(Back);
        }

        RemovedBrushes.PushBack(Workpiece);
    }

    // Remove the originator brushes.
    if (!CommandDelete) CommandDelete=new CommandDeleteT(MapDoc, RemovedBrushes);
    CommandDelete->Do();

    // Select the newly inserted brushes.
    if (!CommandSelect) CommandSelect=CommandSelectT::Set(&MapDoc, InsertedBrushes);
    CommandSelect->Do();

    // Update all observers.
    MapDoc.UpdateAllObservers_Created(InsertedBrushes);

    m_Done=true;

    return true;
}


void CommandClipT::Undo()
{
    wxASSERT(m_Done);

    if (!m_Done) return;

    CommandSelect->Undo();
    CommandDelete->Undo();

    for (unsigned long ClipNr=0; ClipNr<ClipResults.Size(); ClipNr++)
    {
        ArrayT<MapPrimitiveT*> DeletedBrushes;

        if (ClipResults[ClipNr]->Front)
        {
            MapDoc.Remove(ClipResults[ClipNr]->Front);
            DeletedBrushes.PushBack(ClipResults[ClipNr]->Front);
        }

        if (ClipResults[ClipNr]->Back)
        {
            MapDoc.Remove(ClipResults[ClipNr]->Back);
            DeletedBrushes.PushBack(ClipResults[ClipNr]->Back);
        }

        MapDoc.UpdateAllObservers_Deleted(DeletedBrushes);
    }

    m_Done=false;
}


wxString CommandClipT::GetName() const
{
    return "Clip brushes";
}
