/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ReplaceMat.hpp"
#include "Select.hpp"

#include "../ChildFrame.hpp"
#include "../DialogEditSurfaceProps.hpp"
#include "../MapBezierPatch.hpp"
#include "../MapBrush.hpp"
#include "../MapDocument.hpp"
#include "../ToolEditSurface.hpp"
#include "../ToolManager.hpp"

#include "../../EditorMaterial.hpp"
#include "../../GameConfig.hpp"


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

    for (unsigned long i=0; i<Potentials.Size(); i++)
    {
        MapElementT*     Object     =Potentials[i];
        MapBrushT*       Brush      =dynamic_cast<MapBrushT*>(Potentials[i]);
        MapBezierPatchT* BezierPatch=dynamic_cast<MapBezierPatchT*>(Potentials[i]);

        if (!SearchHidden && !Object->IsVisible()) continue;
        if (!Brush && !BezierPatch)                continue;
        if (Brush && !SearchBrushes)               continue;
        if (BezierPatch && !SearchBPatches)        continue;

        if (Brush)       m_Brushes.PushBack(Brush);
        if (BezierPatch) m_BezierPatches.PushBack(BezierPatch);
    }

    if (!MarkOnly)
    {
        m_OldBrushMats.PushBackEmptyExact(m_Brushes.Size());

        for (unsigned int i = 0; i < m_Brushes.Size(); i++)
        {
            MapBrushT*        Brush = m_Brushes[i];
            ArrayT<MapFaceT>& Faces = Brush->GetFaces();

            m_OldBrushMats[i].PushBackEmptyExact(Faces.Size());

            for (unsigned int FaceNr = 0; FaceNr < Faces.Size(); FaceNr++)
                m_OldBrushMats[i][FaceNr] = Faces[FaceNr].GetMaterial();
        }

        m_OldBezierPatchMats.PushBackEmptyExact(m_BezierPatches.Size());

        for (unsigned int i = 0; i < m_BezierPatches.Size(); i++)
            m_OldBezierPatchMats[i] = m_BezierPatches[i]->GetMaterial();
    }
}


CommandReplaceMatT::~CommandReplaceMatT()
{
    delete CommandSelect;
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

    // Just in case faces are selected (which is the case when this command is committed with an active edit surface tool).
    MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ClearSelection();

    // Replace materials.
    for (unsigned int i = 0; i < m_Brushes.Size(); i++)
    {
        MapBrushT* Brush = m_Brushes[i];

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

                        const wxString NewMatName=FaceTexStr.Left(FoundPos)+Replace+FaceTexStr.Right(FaceTexStr.Length()-(FoundPos+Find.Length()));
                        Face.SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(NewMatName, true /*Return dummy if not found.*/));
                    }
                    break;
                }
            }
        }

        if (DoMarkBrush) ObjectsToSelect.PushBack(Brush);
    }

    for (unsigned int i = 0; i < m_BezierPatches.Size(); i++)
    {
        MapBezierPatchT* BezierPatch = m_BezierPatches[i];

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

                    const wxString NewMatName=BPTexStr.Left(FoundPos)+Replace+BPTexStr.Right(BPTexStr.Length()-(FoundPos+Find.Length()));
                    BezierPatch->SetMaterial(MapDoc.GetGameConfig()->GetMatMan().FindMaterial(NewMatName, true /*Return dummy if not found.*/));
                }
                break;
            }
        }
    }

    if (MarkOnly)
    {
        m_ResultString=wxString::Format("%lu materials marked.", NumberReplaced);

        if (!CommandSelect) CommandSelect=CommandSelectT::Set(&MapDoc, ObjectsToSelect);
        CommandSelect->Do();
    }
    else
    {
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
        CommandSelect->Undo();
    }
    else
    {
        for (unsigned int i = 0; i < m_Brushes.Size(); i++)
        {
            MapBrushT*        Brush = m_Brushes[i];
            ArrayT<MapFaceT>& Faces = Brush->GetFaces();

            for (unsigned int FaceNr = 0; FaceNr < Faces.Size(); FaceNr++)
                Faces[FaceNr].SetMaterial(m_OldBrushMats[i][FaceNr]);
        }

        for (unsigned int i = 0; i < m_BezierPatches.Size(); i++)
            m_BezierPatches[i]->SetMaterial(m_OldBezierPatchMats[i]);
    }

    m_Done=false;
}


wxString CommandReplaceMatT::GetName() const
{
    return MarkOnly ? "mark materials" : "replace materials";
}
