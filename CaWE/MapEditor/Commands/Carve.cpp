/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Carve.hpp"
#include "Delete.hpp"

#include "../CompMapEntity.hpp"
#include "../Group.hpp"
#include "../MapBrush.hpp"
#include "../MapDocument.hpp"


using namespace MapEditor;


CommandCarveT::CommandCarveT(MapDocumentT& MapDoc, const ArrayT<const MapBrushT*>& Carvers)
    : m_MapDoc(MapDoc),
      m_Carvers(Carvers),
      m_DeleteCommand(NULL)
{
}


CommandCarveT::~CommandCarveT()
{
    if (!m_Done)
    {
        for (unsigned long BrushNr=0; BrushNr<m_CarvedBrushes.Size(); BrushNr++)
        {
            const ArrayT<MapBrushT*>& Pieces=m_CarvedBrushes[BrushNr];

            for (unsigned long PieceNr=0; PieceNr<Pieces.Size(); PieceNr++)
                delete Pieces[PieceNr];
        }

        for (unsigned long GroupNr=0; GroupNr<m_NewCarveGroups.Size(); GroupNr++)
            delete m_NewCarveGroups[GroupNr];
    }

    // Note: The delete command also deletes the original brushes if they have been removed from the world.
    delete m_DeleteCommand;
}


bool CommandCarveT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    // If carve hasn't been performed (first run), create a carved object for each brush (if they intersect).
    if (m_CarvedBrushes.Size()==0)
    {
        // Build a list of every possibly affected brush in the world.
        ArrayT<MapBrushT*> WorldBrushes;

        ArrayT<MapElementT*> Elems;
        m_MapDoc.GetAllElems(Elems);

        for (unsigned long ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
        {
            MapBrushT* Brush = dynamic_cast<MapBrushT*>(Elems[ElemNr]);

            if (Brush == NULL) continue;                // Skip everything that is not a brush.
            if (m_Carvers.Find(Brush) >= 0) continue;   // Skip brushes that are carvers.

            WorldBrushes.PushBack(Brush);
        }

        // Subtract the carvers from every brush in the world.
        for (unsigned long BrushNr=0; BrushNr<WorldBrushes.Size(); BrushNr++)
        {
            ArrayT<MapBrushT*> Pieces;  // All the pieces resulting from subtracting *all* carvers from brush WorldBrushes[BrushNr].

            // Start with a single piece. Must be a copy of our world brush, because it might get deleted during the loop below.
            MapBrushT* FirstPiece=new MapBrushT(*WorldBrushes[BrushNr]);

            Pieces.PushBack(FirstPiece);

            // Carve each piece by each carver (and handle pieces being carved into sub-pieces correctly).
            for (unsigned long PieceNr=0; PieceNr<Pieces.Size(); PieceNr++)
            {
                for (unsigned long CarverNr=0; CarverNr<m_Carvers.Size(); CarverNr++)
                {
                    ArrayT<MapBrushT*> Outside;

                    if (Pieces[PieceNr]->Subtract(m_Carvers[CarverNr], Outside))
                    {
                        // The carver subtracted something from piece.
                        delete Pieces[PieceNr];
                        Pieces.RemoveAt(PieceNr);   // No need to keep the ordering.
                        Pieces.PushBack(Outside);
                        PieceNr--;
                        break;
                    }
                    else
                    {
                        // No subtraction, delete anything in Outside.
                        for (unsigned long OutNr=0; OutNr<Outside.Size(); OutNr++)
                            delete Outside[OutNr];
                    }
                }
            }

            if (Pieces.Size()==1 && Pieces[0]==FirstPiece)
            {
                // Brush was not affected by the carve.
                delete Pieces[0];
                Pieces.Clear();
                continue;
            }


            // Put the resulting pieces of the carve into proper groups.
            // (It's somewhat unfortunate here that we don't have the concept of hierarchical or sub-groups.)
            if (WorldBrushes[BrushNr]->GetGroup())
            {
                // If the world brush was in a group, make sure that all its pieces are in the same group.
                for (unsigned long PieceNr = 0; PieceNr < Pieces.Size(); PieceNr++)
                    Pieces[PieceNr]->SetGroup(WorldBrushes[BrushNr]->GetGroup());
            }
            else
            {
                // The world brush was not in a group before. If it was carved into at least two pieces,
                // put them into their own new group.
                if (Pieces.Size() > 1)
                {
                    GroupT* Group = new GroupT(m_MapDoc, "carved brush");
                    Group->SelectAsGroup = true;

                    for (unsigned long PieceNr = 0; PieceNr < Pieces.Size(); PieceNr++)
                        Pieces[PieceNr]->SetGroup(Group);

                    m_NewCarveGroups.PushBack(Group);
                }
            }


            m_OriginalBrushes.PushBack(WorldBrushes[BrushNr]);          // The carve operation replaced this brush by...
            m_Parents.PushBack(WorldBrushes[BrushNr]->GetParent());
            m_CarvedBrushes.PushBack(Pieces);                           // ... this set of pieces (zero, one or many).
        }
    }

    // If there are no brushes at all affected by the carve, quit early.
    wxASSERT(m_OriginalBrushes.Size()==m_Parents.Size() && m_Parents.Size()==m_CarvedBrushes.Size());
    if (m_OriginalBrushes.Size()==0) return false;


    // Delete all original brushes (selected brushes will also be deselected by this command).
    // Note that the pointers in OriginalBrushes will stay valid throughout the lifetime of this
    // command so there is no need to clear the array.
    if (!m_DeleteCommand) m_DeleteCommand=new CommandDeleteT(m_MapDoc, m_OriginalBrushes);

    m_DeleteCommand->Do();

    // Add the new groups (if any),
    // and notify all observers that our groups inventory changed.
    if (m_NewCarveGroups.Size()>0)
    {
        m_MapDoc.GetGroups().PushBack(m_NewCarveGroups);
        m_MapDoc.UpdateAllObservers_GroupsChanged();
    }

    // Replace affected brushes by the results of the carve operation.
    ArrayT<MapPrimitiveT*> AllPieces;

    for (unsigned long BrushNr=0; BrushNr<m_OriginalBrushes.Size(); BrushNr++)
    {
        const ArrayT<MapBrushT*>& Pieces=m_CarvedBrushes[BrushNr];

        for (unsigned long PieceNr=0; PieceNr<Pieces.Size(); PieceNr++)
        {
            m_MapDoc.Insert(Pieces[PieceNr], m_Parents[BrushNr]);
            AllPieces.PushBack(Pieces[PieceNr]);    // Collect all pieces in an array that is (1) "flat" and (2) of type "MapElementT*".
        }
    }

    m_MapDoc.UpdateAllObservers_Created(AllPieces);

    m_Done=true;
    return true;
}


void CommandCarveT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    // Remove carved brushes from world.
    // Note that we do not need to use a delete command here, because the carved brushes are never selected
    // and their parent is always the world.
    ArrayT<MapPrimitiveT*> AllPieces;

    for (unsigned long BrushNr=0; BrushNr<m_OriginalBrushes.Size(); BrushNr++)
    {
        const ArrayT<MapBrushT*>& Pieces=m_CarvedBrushes[BrushNr];

        for (unsigned long PieceNr=0; PieceNr<Pieces.Size(); PieceNr++)
        {
            m_MapDoc.Remove(Pieces[PieceNr]);
            AllPieces.PushBack(Pieces[PieceNr]);    // Collect all pieces in an array that is (1) "flat" and (2) of type "MapElementT*".
        }
    }

    m_MapDoc.UpdateAllObservers_Deleted(AllPieces);

    // Remove the new groups again (if any),
    // and notify all observers that our groups inventory changed.
    if (m_NewCarveGroups.Size()>0)
    {
        m_MapDoc.GetGroups().DeleteBack(m_NewCarveGroups.Size());
        m_MapDoc.UpdateAllObservers_GroupsChanged();
    }

    // Restore original brushes.
    m_DeleteCommand->Undo();

    m_Done=false;
}


wxString CommandCarveT::GetName() const
{
    return "Carve";
}
