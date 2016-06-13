/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DialogEditSurfaceProps.hpp"
#include "MapBrush.hpp"
#include "MapBezierPatch.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "ToolbarMaterials.hpp"
#include "ToolEditSurface.hpp"
#include "ToolOptionsBars.hpp"
#include "MapDocument.hpp"

#include "../CommandHistory.hpp"
#include "../CursorMan.hpp"
#include "../ParentFrame.hpp"

#include "Commands/Select.hpp"

#include "wx/wx.h"


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolEditSurfaceT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolEditSurfaceT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolEditSurfaceT::TypeInfo(GetToolTIM(), "ToolEditSurfaceT", "ToolT", ToolEditSurfaceT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolEditSurfaceT::ToolEditSurfaceT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_OptionsBar(new OptionsBar_EditFacePropsToolT(ParentOptionsBar))
{
}


wxWindow* ToolEditSurfaceT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


void ToolEditSurfaceT::OnActivate(ToolT* OldTool)
{
    m_EyeDropperActive=false;

    if (OldTool!=this)
    {
        wxBusyCursor Wait;

        m_MapDoc.GetChildFrame()->ShowPane(m_MapDoc.GetChildFrame()->GetSurfacePropsDialog(), true);
        m_MapDoc.GetChildFrame()->ShowPane(m_MapDoc.GetChildFrame()->GetMaterialsToolbar(), false);

        // Select map elements that are selected in the document also in the "Edit Surfaces" dialog, then clear the
        // original document selection. That is, the selection state is "transferred" from the document to the dialog.
        m_MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ClearSelection();

        for (unsigned long SelNr=0; SelNr<m_MapDoc.GetSelection().Size(); SelNr++)
            m_MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ToggleClick(m_MapDoc.GetSelection()[SelNr], EditSurfacePropsDialogT::ALL_FACES);

        m_MapDoc.CompatSubmitCommand(CommandSelectT::Clear(&m_MapDoc));
    }
}


void ToolEditSurfaceT::OnDeactivate(ToolT* NewTool)
{
    if (NewTool!=this)
    {
        m_MapDoc.GetChildFrame()->ShowPane(m_MapDoc.GetChildFrame()->GetSurfacePropsDialog(), false);
        m_MapDoc.GetChildFrame()->ShowPane(m_MapDoc.GetChildFrame()->GetMaterialsToolbar(), true);

        // Remove all faces from the dialog's list and update their selection state to be not selected.
        m_MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ClearSelection();
    }
}


bool ToolEditSurfaceT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const ArrayT<MapElementT*> Hits=ViewWindow.GetElementsAt(ME.GetPosition());

    if (!ME.ControlDown())
        m_MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ClearSelection();

    for (unsigned long HitNr=0; HitNr<Hits.Size(); HitNr++)
    {
        m_MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ToggleClick(Hits[HitNr], EditSurfacePropsDialogT::ALL_FACES);

        // When something is newly selected (Control is not down), its surface properties are also picked up.
        if (HitNr==0 && !ME.ControlDown())
            ViewWindow.GetChildFrame()->GetSurfacePropsDialog()->EyeDropperClick(Hits[HitNr], 0);
    }

    return true;
}


bool ToolEditSurfaceT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ALT:
            // Activate eyedropper mode here. Cursor will be set as soon as it is moved into
            // the 3D view.
            m_EyeDropperActive=true;
            return true;

        default:
            return false;
    }
}


bool ToolEditSurfaceT::OnKeyUp2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ALT:
            // Deactivate eyedropper mode here. Cursor will be reset as soon as it is moved into
            // the 3D view.
            m_EyeDropperActive=false;
            return true;

        default:
            return false;
    }
}


bool ToolEditSurfaceT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ALT:
            m_EyeDropperActive=true;
            ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EYE_DROPPER));
            return true;

        default:
            return false;
    }
}


bool ToolEditSurfaceT::OnKeyUp3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ALT:
            m_EyeDropperActive=false;
            ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::EDIT_FACEPROPS_TOOL));
            return true;

        default:
            return false;
    }
}


bool ToolEditSurfaceT::OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    const ArrayT<ViewWindow3DT::HitInfoT> Hits=ViewWindow.GetElementsAt(ME.GetPosition());

    // Having this statement here means that the user absolutely cannot entirely clear the
    // surfaces selection (in 3D views, it works in 2D views), but that is just fine - why would he?
    if (Hits.Size()==0) return true;

    MapElementT*  Object   =Hits[0].Object;
    unsigned long FaceIndex=Hits[0].FaceNr;

    if (m_EyeDropperActive)
    {
        ViewWindow.GetChildFrame()->GetSurfacePropsDialog()->EyeDropperClick(Object, FaceIndex);
        return true;
    }

    if (!ME.ControlDown())
        ViewWindow.GetChildFrame()->GetSurfacePropsDialog()->ClearSelection();

    ViewWindow.GetChildFrame()->GetSurfacePropsDialog()->ToggleClick(Object, ME.ShiftDown() ? EditSurfacePropsDialogT::ALL_FACES : FaceIndex);

    // When something is newly selected (Control is not down), its surface properties are also picked up.
    if (!ME.ControlDown())
        ViewWindow.GetChildFrame()->GetSurfacePropsDialog()->EyeDropperClick(Object, FaceIndex);

    return true;
}


bool ToolEditSurfaceT::OnRMouseClick3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    const ArrayT<ViewWindow3DT::HitInfoT> Hits=ViewWindow.GetElementsAt(ME.GetPosition());

    // This is in the RMB *up* instead of the RMB *down* handler in order to not have the context menu shown
    // when the user hit something for an apply-click.
    if (Hits.Size()>0)
    {
        ViewWindow.GetChildFrame()->GetSurfacePropsDialog()->ApplyClick(ViewWindow, Hits[0].Object, ME.ShiftDown() ? EditSurfacePropsDialogT::ALL_FACES : Hits[0].FaceNr);
        return true;
    }

    return false;
}


bool ToolEditSurfaceT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.SetCursor(CursorMan->GetCursor(m_EyeDropperActive ? CursorManT::EYE_DROPPER : CursorManT::EDIT_FACEPROPS_TOOL));

    return true;
}


bool ToolEditSurfaceT::UpdateStatusBar(ChildFrameT* ChildFrame) const
{
    ChildFrame->SetStatusText(wxString::Format(" %lu faces selected.", ChildFrame->GetSurfacePropsDialog()->GetNrOfSelectedFaces()), ChildFrameT::SBP_SELECTION);
    ChildFrame->SetStatusText("", ChildFrameT::SBP_SELECTION_DIMS);
    return true;
}
