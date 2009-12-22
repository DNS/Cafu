/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#include "ReplaceMat.hpp"
#include "Select.hpp"

#include "../MapDocument.hpp"
#include "../MapBrush.hpp"
#include "../EditorMaterial.hpp"
#include "../GameConfig.hpp"
#include "../ChildFrame.hpp"
#include "../ToolEditSurface.hpp"
#include "../ToolManager.hpp"
#include "../DialogEditSurfaceProps.hpp"


CommandReplaceMatT::CommandReplaceMatT(MapDocumentT& MapDoc_, const ArrayT<MapElementT*>& Selection, const wxString& Find_, const wxString& Replace_, ReplaceActionT Action_, bool MarkOnly_, bool SearchSelection, bool SearchBrushes, bool SearchBPatches, bool SearchHidden)
    : MapDoc(MapDoc_),
      Find(Find_),
      Replace(Replace_),
      Action(Action_),
      MarkOnly(MarkOnly_),
      CommandSelect(NULL),
      m_ResultString("")
{
    wxASSERT(Action==ExactMatches || Action==PartialMatchesFull || Action==PartialMatchesSubst);

    // Build array of potential elements to replace materials in.
    ArrayT<MapElementT*> Potentials;

    if (SearchSelection) Potentials=Selection;
                    else MapDoc.GetAllElems(Potentials);

    // Further narrow down results and store them in ReplaceMatObjects.
    for (unsigned long i=0; i<Potentials.Size(); i++)
    {
        MapElementT*     Object     =Potentials[i];
        MapBrushT*       Brush      =dynamic_cast<MapBrushT*>(Potentials[i]);
        MapBezierPatchT* BezierPatch=dynamic_cast<MapBezierPatchT*>(Potentials[i]);

        if (!SearchHidden && !Object->IsVisible()) continue;
        if (!Brush && !BezierPatch)                continue;
        if (Brush && !SearchBrushes)               continue;
        if (BezierPatch && !SearchBPatches)        continue;

        ReplaceMatObjects.PushBack(Object);
    }
}


CommandReplaceMatT::~CommandReplaceMatT()
{
    delete CommandSelect;

    for (unsigned long i=0; i<OldStates.Size(); i++)
        delete OldStates[i];

    OldStates.Clear();
}


const wxString& CommandReplaceMatT::GetResultString() const
{
    return m_ResultString;
}


bool CommandReplaceMatT::Do()
{
    wxASSERT(!m_Done);
    if (m_Done) return false;

    wxBusyCursor         BusyCursor;
    unsigned long        NumberReplaced=0;
    ArrayT<MapElementT*> ObjectsToSelect;   // Objects to select at the end of Do().

    // Just in case faces are selected in the memento (which is the case when this command is comitted with an active edit surface tool).
    MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ClearSelection();

    // Replace materials.
    for (unsigned long i=0; i<ReplaceMatObjects.Size(); i++)
    {
        MapBrushT* Brush=dynamic_cast<MapBrushT*>(ReplaceMatObjects[i]);
        if (Brush)
        {
            bool Saved      =false;
            bool DoMarkBrush=false;

            for (unsigned long FaceNr=0; FaceNr<Brush->GetFaces().Size(); FaceNr++)
            {
                MapFaceT& Face      =Brush->GetFaces()[FaceNr];
                wxString  FaceTexStr=Face.GetMaterial()->GetName();

                switch (Action)
                {
                    case ExactMatches:
                    {
                        if (FaceTexStr.Lower()==Find.Lower())
                        {
                            NumberReplaced++;

                            if (MarkOnly)
                            {
                                if (MapDoc.GetChildFrame()->GetToolManager().GetActiveToolType()==&ToolEditSurfaceT::TypeInfo)
                                {
                                    MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ToggleClick(Brush, FaceNr);
                                }
                                else
                                {
                                    DoMarkBrush=true;
                                }
                                break;
                            }

                            if (!Saved)
                            {
                                Saved=true;
                                OldStates.PushBack(new MapBrushT(*Brush));
                            }

                            Face.SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(Replace, true /*Return dummy if not found.*/));
                        }
                        break;
                    }

                    case PartialMatchesFull:
                    {
                        if (FaceTexStr.Lower().Find(Find.Lower())!=-1)
                        {
                            NumberReplaced++;

                            if (MarkOnly)
                            {
                                if (MapDoc.GetChildFrame()->GetToolManager().GetActiveToolType()==&ToolEditSurfaceT::TypeInfo)
                                {
                                    MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ToggleClick(Brush, FaceNr);
                                }
                                else
                                {
                                    DoMarkBrush=true;
                                }
                                break;
                            }

                            if (!Saved)
                            {
                                Saved=true;
                                OldStates.PushBack(new MapBrushT(*Brush));
                            }

                            Face.SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(Replace, true /*Return dummy if not found.*/));
                        }
                        break;
                    }

                    case PartialMatchesSubst:
                    {
                        const int FoundPos=FaceTexStr.Lower().Find(Find.Lower());

                        if (FoundPos!=-1)
                        {
                            NumberReplaced++;

                            if (MarkOnly)
                            {
                                if (MapDoc.GetChildFrame()->GetToolManager().GetActiveToolType()==&ToolEditSurfaceT::TypeInfo)
                                {
                                    MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ToggleClick(Brush, FaceNr);
                                }
                                else
                                {
                                    DoMarkBrush=true;
                                }
                                break;
                            }

                            if (!Saved)
                            {
                                Saved=true;
                                OldStates.PushBack(new MapBrushT(*Brush));
                            }

                            const wxString NewMatName=FaceTexStr.Left(FoundPos)+Replace+FaceTexStr.Right(FaceTexStr.Length()-(FoundPos+Find.Length()));
                            Face.SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(NewMatName, true /*Return dummy if not found.*/));
                        }
                        break;
                    }
                }
            }

            if (!MarkOnly && !Saved) OldStates.PushBack(NULL);
            if (DoMarkBrush) ObjectsToSelect.PushBack(Brush);
        }


        MapBezierPatchT* BezierPatch=dynamic_cast<MapBezierPatchT*>(ReplaceMatObjects[i]);
        if (BezierPatch)
        {
            switch (Action)
            {
                case ExactMatches:
                {
                    if (BezierPatch->GetMaterial()->GetName().Lower()==Find.Lower())
                    {
                        NumberReplaced++;

                        if (MarkOnly)
                        {
                            ObjectsToSelect.PushBack(BezierPatch);
                            break;
                        }

                        OldStates.PushBack(new MapBezierPatchT(*BezierPatch));
                        BezierPatch->SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(Replace, true /*Return dummy if not found.*/));
                    }
                    break;
                }

                case PartialMatchesFull:
                {
                    if (BezierPatch->GetMaterial()->GetName().Lower().Find(Find.Lower())!=-1)
                    {
                        NumberReplaced++;

                        if (MarkOnly)
                        {
                            ObjectsToSelect.PushBack(BezierPatch);
                            break;
                        }

                        OldStates.PushBack(new MapBezierPatchT(*BezierPatch));
                        BezierPatch->SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(Replace, true /*Return dummy if not found.*/));
                    }
                    break;
                }

                case PartialMatchesSubst:
                {
                    wxString BPTexStr=BezierPatch->GetMaterial()->GetName();
                    int      FoundPos=BPTexStr.Lower().Find(Find.Lower());

                    if (FoundPos!=-1)
                    {
                        NumberReplaced++;

                        if (MarkOnly)
                        {
                            ObjectsToSelect.PushBack(BezierPatch);
                            break;
                        }

                        OldStates.PushBack(new MapBezierPatchT(*BezierPatch));
                        const wxString NewMatName=BPTexStr.Left(FoundPos)+Replace+BPTexStr.Right(BPTexStr.Length()-(FoundPos+Find.Length()));
                        BezierPatch->SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(NewMatName, true /*Return dummy if not found.*/));
                    }
                    break;
                }
            }

            // If we're really replacing (not just marking only), and no "old state" has been created for BezierPatch / ReplaceMatObjects[i]
            // because the material of BezierPatch was not replaced, we have to create a NULL "old state" here.
            if (!MarkOnly && OldStates.Size()==i) OldStates.PushBack(NULL);     // OldStates.Size()==i+1 is correct.
        }
    }

    if (MarkOnly)
    {
        wxASSERT(OldStates.Size()==0);
        m_ResultString=wxString::Format("%lu materials marked.", NumberReplaced);

        if (!CommandSelect) CommandSelect=CommandSelectT::Set(&MapDoc, ObjectsToSelect);
        CommandSelect->Do();
    }
    else
    {
        wxASSERT(ReplaceMatObjects.Size()==OldStates.Size());
        m_ResultString=wxString::Format("%lu materials replaced.", NumberReplaced);

        if (NumberReplaced==0) return false; // Nothing done so return false (the command won't be put into the history).
    }

    m_Done=true;
    return true;
}


void CommandReplaceMatT::Undo()
{
    wxASSERT(m_Done);
    if (!m_Done) return;

    if (MarkOnly)
    {
        wxASSERT(OldStates.Size()==0);

        CommandSelect->Undo();
    }
    else
    {
        for (unsigned long i=0; i<ReplaceMatObjects.Size(); i++)
            if (OldStates[i]) ReplaceMatObjects[i]->Assign(OldStates[i]);

        // These are rebuilt on Do().
        for (unsigned long i=0; i<OldStates.Size(); i++)
            delete OldStates[i];

        OldStates.Clear();
    }

    m_Done=false;
}


wxString CommandReplaceMatT::GetName() const
{
    return MarkOnly ? "mark materials" : "replace materials";
}
