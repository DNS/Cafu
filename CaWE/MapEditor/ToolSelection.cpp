/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompMapEntity.hpp"
#include "DialogInspector.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "MapModel.hpp"
#include "MapPlant.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "Renderer2D.hpp"
#include "ToolOptionsBars.hpp"
#include "ToolSelection.hpp"
#include "ToolManager.hpp"
#include "DialogEditSurfaceProps.hpp"

#include "../Camera.hpp"
#include "../CommandHistory.hpp"
#include "../CursorMan.hpp"
#include "../GameConfig.hpp"
#include "../Options.hpp"

#include "Commands/AddPrim.hpp"
#include "Commands/NewEntity.hpp"
#include "Commands/Transform.hpp"
#include "Commands/Select.hpp"

#include "Plants/PlantDescription.hpp"

#include "wx/wx.h"
#include "wx/filename.h"


using namespace MapEditor;


// TODO:
//   - RMB / Context menu behaviour
//   - Selection behaviour: New selection, add to selection, remove from selection, toggle selection. Via modifiers (ALT, SHIFT, CTRL)? Via MMB / RMB ??
//   - Remove view2d.SelectByHandles?    Select by: entire object / handle / part of object ?
//   - Make sure that pressing SHIFT properly adds the small "+" to the "sizing" cursor.
//   - Die m_TrafoBox mit den richtigen Werten initialisieren im LMB-*down* handler, obwohl es erstmal in den UNDECIDED state geht
//   - Fix improperly invalidated drawing rect, see http://www.cafu.de/forum/viewtopic.php?p=3255#p3255 for details.


CycleHitsTimerT::CycleHitsTimerT(ToolSelectionT& SelectionTool)
    : m_SelectionTool(SelectionTool)
{
}


void CycleHitsTimerT::Notify()
{
    static bool IsCycling=false;

    if (IsCycling) return;

    IsCycling=true;
    m_SelectionTool.StepCurHitNr(+1);
    IsCycling=false;
}


/*** Begin of TypeSys related definitions for this class. ***/

void* ToolSelectionT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    const ToolCreateParamsT* TCPs=static_cast<const ToolCreateParamsT*>(&Params);

    return new ToolSelectionT(TCPs->MapDoc, TCPs->ToolMan, TCPs->ParentOptionsBar);
}

const cf::TypeSys::TypeInfoT ToolSelectionT::TypeInfo(GetToolTIM(), "ToolSelectionT", "ToolT", ToolSelectionT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


ToolSelectionT::ToolSelectionT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar)
    : ToolT(MapDoc, ToolMan),
      m_ToolState(TS_IDLE),
      m_CycleHitsTimer(*this),
      m_HitList(),
      m_CurHitNr(-1),
      m_OptionsBar(new OptionsBar_SelectionToolT(ParentOptionsBar))
{
    UpdateTrafoBox();
    m_MapDoc.RegisterObserver(this);
}


ToolSelectionT::~ToolSelectionT()
{
    m_MapDoc.UnregisterObserver(this);
}


wxWindow* ToolSelectionT::GetOptionsBar()
{
    // Cannot define this method inline in the header file, because there the compiler
    // does not yet know that the type of m_OptionsBar is in fact related to wxWindow.
    return m_OptionsBar;
}


void ToolSelectionT::OnActivate(ToolT* OldTool)
{
    UpdateTrafoBox();
    m_TrafoBox.SetTrafoMode(TrafoBoxT::TM_SCALE);
}


void ToolSelectionT::OnDeactivate(ToolT* NewTool)
{
}


bool ToolSelectionT::OnKeyDown2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
            OnEscape(ViewWindow);
            return true;

        case WXK_UP:
        case WXK_DOWN:
        case WXK_LEFT:
        case WXK_RIGHT:
            if (!KE.ShiftDown()) break;
            if (m_MapDoc.GetSelection().Size()>0) NudgeSelection(ViewWindow.GetAxesInfo(), KE);
            return true;

        case WXK_PAGEDOWN: StepCurHitNr(+1); return true;
        case WXK_PAGEUP:   StepCurHitNr(-1); return true;

        case WXK_SHIFT:
            // TODO!
            // If the user is currently moving an element and presses shift, switch to "plus sizing" cursor.
            // if (...) ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::SIZING_PLUS));
            return true;
    }

    return false;
}


bool ToolSelectionT::OnKeyUp2D(ViewWindow2DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_SHIFT:
            // TODO!
            // If the user is currently moving an element and presses shift, switch to "plus sizing" cursor.
            // if (...) ViewWindow.SetCursor(wxCursor(wxCURSOR_SIZING));
            return true;
    }

    return false;
}


bool ToolSelectionT::OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    ViewWindow.CaptureMouse();

    m_LDownPosWin  =ME.GetPosition();
    m_LDownPosWorld=m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), -1 /*Snap all axes.*/);
    m_LDragPosWorld=m_LDownPosWorld;

    m_LDownPosWorld[ViewWindow.GetAxesInfo().ThirdAxis]=-999999.0f;
    m_LDragPosWorld[ViewWindow.GetAxesInfo().ThirdAxis]= 999999.0f;

    wxASSERT(m_ToolState==TS_IDLE);
    m_ToolState=TS_IDLE;

    // Is the click inside the transformation box ("over the body"), or one of its handles?
    switch (m_TrafoBox.CheckForHandle(ViewWindow, ViewWindow.WindowToTool(ME.GetPosition())))
    {
        case TrafoBoxT::TH_NONE:
            // Cursor is not over the trafo box at all.
            // Just break here in order to deal with other options below.
            break;

        case TrafoBoxT::TH_BODY:
            // Cursor is over the body of the trafo box.
            // We don't know yet if this is the beginning of a box translation (movement),
            // or just a cycle of the transformation mode (scale, rotation, or shear).
            m_ToolState=TS_UNDECIDED;
            return true;

        default:
            // Cursor is over one of the transformation handles.
            m_TrafoBox.BeginTrafo(ViewWindow, ViewWindow.WindowToTool(ME.GetPosition()));
            m_ToolState=TS_BOX_TRAFO;
            return true;
    }

    // The click is not related to the transformation box, now check if a one-click selection is possible.
    ArrayT<MapElementT*> ClickedElems=ViewWindow.GetElementsAt(ME.GetPosition());

    // Filter out any locked ("cannot select") elements.
    // (Invisible elements have already been filtered out by the GetElementsAt() method.)
    for (unsigned long ElemNr = 0; ElemNr < ClickedElems.Size(); ElemNr++)
        if (!ClickedElems[ElemNr]->CanSelect())
        {
            ClickedElems.RemoveAt(ElemNr);
            ElemNr--;
        }

    if (ClickedElems.Size() == 0)
    {
        // Click was in empty space, but we don't know yet if it will become dragging a box/frame for selection,
        // or just a click to clear the selection.
        m_ToolState=TS_UNDECIDED;
        return true;
    }

    // It's a one-click selection of the elements under the cursor.
    SetHitList(ClickedElems, ME.ControlDown());
    m_ToolState=TS_POINT_SEL;
    return true;
}


bool ToolSelectionT::OnLMouseUp2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    m_CycleHitsTimer.Stop();

    if (!ViewWindow.HasCapture()) return false;     // Drag started outside of ViewWindow.
    ViewWindow.ReleaseMouse();

    // This is probably overkill, but failing here means the user *will* loose his map (no way to quit CaWE but via the Task-Man).
    for (unsigned long TestNr=0; ViewWindow.HasCapture() && TestNr<20; TestNr++)
    {
        wxMessageBox(wxString::Format("Hmmm. I tried to release mouse capture, but still get HasCapture()==true.\nTry %lu of 20.", TestNr+1));
        ViewWindow.ReleaseMouse();
    }
    wxASSERT(!ViewWindow.HasCapture());


    switch (m_ToolState)
    {
        case TS_IDLE:
        {
            // This event can occur when m_ToolState is TS_IDLE when the drag started outside of ViewWindow,
            // and when the user pressed ESC for aborting any pending operations.
            return false;
        }

        case TS_UNDECIDED:
        {
            if (m_TrafoBox.CheckForHandle(ViewWindow, ViewWindow.WindowToTool(m_LDownPosWin))==TrafoBoxT::TH_BODY)
            {
                m_TrafoBox.SetNextTrafoMode();
            }
            else
            {
                // It was a click in empty space, so just clear the selection, taking the CTRL key into account.
                SetHitList(ArrayT<MapElementT*>(), ME.ControlDown());
            }
            break;
        }

        case TS_POINT_SEL:
        {
            // The timer for cycling the hits has already been stopped above.
            break;
        }

        case TS_DRAG_SEL:
        {
            // Select (or toggle) all elements in the dragged rectangle.
            const BoundingBox3fT       DragBB(m_LDownPosWorld, m_LDragPosWorld);
            const bool                 InsideOnly = ME.ShiftDown();
            const ArrayT<MapElementT*> ElemsInBB = m_MapDoc.GetElementsIn(DragBB, InsideOnly, Options.view2d.SelectByHandles);

            ArrayT<MapElementT*> RemoveFromSel;
            ArrayT<MapElementT*> AddToSel;

            for (unsigned long ElemNr=0; ElemNr<ElemsInBB.Size(); ElemNr++)
            {
                MapElementT* Elem = ElemsInBB[ElemNr];

                // Skip hidden (invisible) elements.
                if (!Elem->IsVisible()) continue;

                // Skip locked ("cannot select") elements.
                if (!Elem->CanSelect()) continue;

                // Compute the consequences of toggling the element.
                Elem->GetToggleEffects(RemoveFromSel, AddToSel, m_MapDoc.GetAutoGroupEntities());
            }

            if (RemoveFromSel.Size()>0 || AddToSel.Size()>0)
            {
                // Clear the hit list.
                m_HitList.Overwrite();
                m_CurHitNr=-1;

                if (!ME.ControlDown())
                {
                    // Set a new selection "from scratch" (Control is not down).
                    AddToSel.PushBack(RemoveFromSel);

                    m_MapDoc.CompatSubmitCommand(CommandSelectT::Set(&m_MapDoc, AddToSel));
                }
                else
                {
                    // Control is down, toggle the relevant elements.
                    if (RemoveFromSel.Size() > 0) m_MapDoc.CompatSubmitCommand(CommandSelectT::Remove(&m_MapDoc, RemoveFromSel));
                    if (     AddToSel.Size() > 0) m_MapDoc.CompatSubmitCommand(CommandSelectT::   Add(&m_MapDoc,      AddToSel));
                }
            }
            break;
        }

        case TS_BOX_TRAFO:
        {
            // IMPORTANT NOTE:
            // The order of the statements below is critical and cannot be changed:
            //   1. m_TrafoBox.GetTrafoCommand() must be called before m_TrafoBox.FinishTrafo(),
            //      because commands can only be obtained while the transformation is still active.
            //   2. m_TrafoBox.FinishTrafo() must be called before SubmitCommand(), because SubmitCommand()
            //      will eventually update the selection (UpdateTrafoBox() gets called), which is only
            //      allowed when *no* transformation is active.

            // Transform the selected map elements, possibly cloning if SHIFT is pressed.
            wxASSERT(m_TrafoBox.GetDragState() != TrafoBoxT::TH_NONE);   // A requirement for calling TrafoBoxT::GetTrafoCommand().

            CommandT* TrafoCmd = NULL;

            if (ME.ShiftDown())
            {
                TrafoCmd = CloneDrag();

                // Now finish the box transformation (makes m_TrafoBox.GetDragState() return TrafoBoxT::TH_NONE again).
                m_TrafoBox.FinishTrafo();

                // I don't understand exactly why, but we have to atomically set both the m_TrafoBox as well as the m_ToolState here
                // (m_ToolState is set to TS_IDLE universally below again, which normally appears to be sufficient).
                // However, it seems that somewhere in the call to SubmitCommand(), wxMSW finds an opportunity to dispatch more
                // (mouse) events; so without the following line, we enter OnMouseMove2D() in m_ToolState TS_BOX_TRAFO while
                // m_TrafoBox.GetDragState() yields the mismatching TH_NONE.
                m_ToolState = TS_IDLE;
            }
            else
            {
                TrafoCmd = m_TrafoBox.GetTrafoCommand(m_MapDoc, Options.general.LockingTextures);

                // Now finish the box transformation (makes m_TrafoBox.GetDragState() return TrafoBoxT::TH_NONE again).
                m_TrafoBox.FinishTrafo();

                // I don't understand exactly why, but we have to atomically set both the m_TrafoBox as well as the m_ToolState here
                // (m_ToolState is set to TS_IDLE universally below again, which normally appears to be sufficient).
                // However, it seems that somewhere in the call to SubmitCommand(), wxMSW finds an opportunity to dispatch more
                // (mouse) events; so without the following line, we enter OnMouseMove2D() in m_ToolState TS_BOX_TRAFO while
                // m_TrafoBox.GetDragState() yields the mismatching TH_NONE.
                m_ToolState = TS_IDLE;
            }

            if (TrafoCmd) m_MapDoc.CompatSubmitCommand(TrafoCmd);
            break;
        }
    }

    // Releasing the button finishes any pending operations, so finally revert to idle tool state.
    m_ToolState=TS_IDLE;

    // Whether it's strictly necessary or not - unconditionally update all views.
    m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
    return true;
}


bool ToolSelectionT::OnMouseMove2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME)
{
    const wxPoint MousePosTS=ViewWindow.WindowToTool(ME.GetPosition());

    m_LDragPosWorld=m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(ME.GetPosition(), 0.0f), ME.AltDown(), ViewWindow.GetAxesInfo().ThirdAxis);
    m_LDragPosWorld[ViewWindow.GetAxesInfo().ThirdAxis]=999999.0f;

    switch (m_ToolState)
    {
        case TS_IDLE:
        {
            // Run a "null Trafo" in order to update the m_TrafoBox's internal values.
            // The m_TrafoBox then uses this information for rendering: If we are idle, but
            // TH_BODY dragging *would* be possible, the related RefPos hints are rendered
            // according to the current mouse position.
            m_TrafoBox.BeginTrafo(ViewWindow, MousePosTS);
            m_TrafoBox.FinishTrafo();

            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);

            // Determine the objects under the cursor, and update it as required.
            // If there is a transformation box, it takes precedence.
            const TrafoBoxT::TrafoHandleT TrafoHandle=m_TrafoBox.CheckForHandle(ViewWindow, MousePosTS);

            if (TrafoHandle!=TrafoBoxT::TH_NONE)
            {
                // Ok, there is a selection (and thus a valid m_TrafoBox),
                // and the cursor is over its body or over a valid handle.
                ViewWindow.SetCursor(m_TrafoBox.SuggestCursor(TrafoHandle));
                break;
            }

            // If a one-click selection is possible here, set the cursor to a cross.
            // (GetElementsAt() returns visible elements only. If at least one of those is not-locked, selection is possible.)
            const ArrayT<MapElementT*> Elems=ViewWindow.GetElementsAt(ME.GetPosition());
            bool                       CanSelect=false;

            for (unsigned long ElemNr=0; ElemNr<Elems.Size(); ElemNr++)
                if (Elems[ElemNr]->CanSelect())
                {
                    CanSelect=true;
                    break;
                }

            if (CanSelect)
            {
                ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::CROSS));
                break;
            }

            // Cursor is over empty space, so set the standard shape.
            ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
            break;
        }

        case TS_UNDECIDED:
        {
            // If the user dragged beyond the drag threshold, switch to the next state.
            const wxPoint Drag      =m_LDownPosWin-ME.GetPosition();
            const wxPoint LDownPosTS=ViewWindow.WindowToTool(m_LDownPosWin);

            if (abs(Drag.x)<4 && abs(Drag.y)<4) break;

            if (m_TrafoBox.CheckForHandle(ViewWindow, LDownPosTS)==TrafoBoxT::TH_BODY)
            {
                // Start translating the selection.
                const bool Result = m_TrafoBox.BeginTrafo(ViewWindow, LDownPosTS);

                wxASSERT(Result);
                wxASSERT(m_TrafoBox.GetDragState()==TrafoBoxT::TH_BODY);

                m_ToolState=Result ? TS_BOX_TRAFO : TS_IDLE;
            }
            else
            {
                // Start dragging a new frame for selecting a set of objects.
                m_ToolState=TS_DRAG_SEL;
            }

            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            break;
        }

        case TS_POINT_SEL:
        {
            // A point selection click was performed on LMB down, and we're currently
            // cycling through the subsets of the selected objects.
            // No further action required here.
            break;
        }

        case TS_DRAG_SEL:
        {
            // Update the box that the user is dragging for selecting a set of map elements.
            // The m_LDragPosWorld member has already been updated above, so there is not much left to do here.
            m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            break;
        }

        case TS_BOX_TRAFO:
        {
            // The box is currently being transformed - update accordingly.
            wxASSERT(m_TrafoBox.GetDragState()!=TrafoBoxT::TH_NONE);

            if (m_TrafoBox.UpdateTrafo(ViewWindow, MousePosTS, ME.AltDown()))
            {
                m_ToolMan.UpdateAllObservers(this, UPDATE_NOW);
            }

            // Set cursor to "sizing plus" cursor if clone dragging an object.
            if (ME.ShiftDown() && m_TrafoBox.GetDragState()==TrafoBoxT::TH_BODY) ViewWindow.SetCursor(CursorMan->GetCursor(CursorManT::SIZING_PLUS));
            break;
        }
    }

    return true;
}


int ToolSelectionT::OnContextMenu2D(ViewWindow2DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu)
{
    if (m_ToolState!=TS_IDLE) return wxID_NONE;

    wxMenu* SubMenuCreate=new wxMenu();
    SubMenuCreate->Append(ID_CREATE_MODEL, "Model");
    SubMenuCreate->Append(ID_CREATE_PLANT, "Plant");

    if (Menu.GetMenuItemCount()>0) Menu.AppendSeparator();
    Menu.AppendSubMenu(SubMenuCreate, "Create");

    const int MenuSelID=ViewWindow.GetPopupMenuSelectionFromUser(Menu);

    if (MenuSelID>=ID_CREATE_MODEL)
    {
        wxPoint MousePosWin=ViewWindow.ScreenToClient(CE.GetPosition());

        if (CE.GetPosition()==wxDefaultPosition)
        {
            MousePosWin=wxPoint(ViewWindow.GetClientSize().GetWidth()/2, ViewWindow.GetClientSize().GetHeight()/2);
        }

        const Vector3fT WorldPos=m_MapDoc.SnapToGrid(ViewWindow.WindowToWorld(MousePosWin, m_MapDoc.GetMostRecentSelBB().Min.z), false /*Toogle?*/, -1 /*Snap all axes.*/);

        switch (MenuSelID)
        {
            case ID_CREATE_MODEL: CreateModel(WorldPos); break;
            case ID_CREATE_PLANT: CreatePlant(WorldPos); break;
        }
    }

    return MenuSelID;
}


bool ToolSelectionT::OnKeyDown3D(ViewWindow3DT& ViewWindow, wxKeyEvent& KE)
{
    switch (KE.GetKeyCode())
    {
        case WXK_ESCAPE:
            OnEscape(ViewWindow);
            return true;

        case WXK_UP:
        case WXK_DOWN:
        case WXK_LEFT:
        case WXK_RIGHT:
            if (!KE.ShiftDown()) break;
            if (m_MapDoc.GetSelection().Size()>0) NudgeSelection(ViewWindow.GetAxesInfo(), KE);
            return true;

        case WXK_PAGEDOWN: StepCurHitNr(+1); return true;
        case WXK_PAGEUP:   StepCurHitNr(-1); return true;
    }

    return false;
}


bool ToolSelectionT::OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    if (ME.ButtonDClick())
    {
        InspectorDialogT* Insp=m_MapDoc.GetChildFrame()->GetInspectorDialog();

        if (!m_MapDoc.GetChildFrame()->IsPaneShown(Insp))
        {
            m_MapDoc.GetChildFrame()->ShowPane(Insp);
        }

        return true;
    }

    // Determine how many (and which) map elements are under the clicked pixel.
    const ArrayT<ViewWindow3DT::HitInfoT> HitInfos=ViewWindow.GetElementsAt(ME.GetPosition());

    // Unfortunately we have to forego the cycling feature when the nearest hit element is locked
    // (it could easily be re-enabled by deleting the following "if (...) return true;" clause).
    // This is because SetHitList() immediately clears the selection (when Control is not pressed),
    // and thus the user experiences a loss of the old selection when nothing at all should happen.
    if (HitInfos.Size()>0 && !HitInfos[0].Object->CanSelect()) return true;

    // Note that locked ("cannot select") elements are *not* filtered out here.
    // (Invisible elements have already been filtered out by the GetElementsAt() method.)
    // It is important to also pass locked elements to SetHitList() (which properly deals with them),
    // because otherwise the user feels that he can click and select "through" locked elements.
    ArrayT<MapElementT*> HitElems;

    for (unsigned long HitNr=0; HitNr<HitInfos.Size(); HitNr++)
        HitElems.PushBack(HitInfos[HitNr].Object);

    SetHitList(HitElems, ME.ControlDown());
    return true;
}


bool ToolSelectionT::OnLMouseUp3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    m_CycleHitsTimer.Stop();
    return true;
}


bool ToolSelectionT::OnMouseMove3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME)
{
    // TODO: Implement some sophisticated rules for setting the cursor shape?
    ViewWindow.SetCursor(*wxSTANDARD_CURSOR);
    return true;
}


int ToolSelectionT::OnContextMenu3D(ViewWindow3DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu)
{
    if (m_ToolState!=TS_IDLE) return wxID_NONE;

    wxMenu* SubMenuCreate=new wxMenu();
    SubMenuCreate->Append(ID_CREATE_MODEL, "Model");
    SubMenuCreate->Append(ID_CREATE_PLANT, "Plant");

    if (Menu.GetMenuItemCount()>0) Menu.AppendSeparator();
    Menu.AppendSubMenu(SubMenuCreate, "Create");

    const int MenuSelID=ViewWindow.GetPopupMenuSelectionFromUser(Menu);

    if (MenuSelID>=ID_CREATE_MODEL)
    {
        wxPoint MousePosWin=ViewWindow.ScreenToClient(CE.GetPosition());

        if (CE.GetPosition()==wxDefaultPosition)
        {
            MousePosWin=wxPoint(ViewWindow.GetClientSize().GetWidth()/2, ViewWindow.GetClientSize().GetHeight()/2);
        }

        const ArrayT<ViewWindow3DT::HitInfoT> Hits=ViewWindow.GetElementsAt(MousePosWin);
        const Vector3fT ViewDir=ViewWindow.WindowToWorld(MousePosWin)-ViewWindow.GetCamera().Pos;
        const Vector3fT HitPos =Hits.Size()>0 ? Hits[0].Pos : ViewWindow.GetCamera().Pos + ViewDir*8.0f;

        switch (MenuSelID)
        {
            case ID_CREATE_MODEL: CreateModel(HitPos); break;
            case ID_CREATE_PLANT: CreatePlant(HitPos); break;
        }
    }

    return MenuSelID;
}


void ToolSelectionT::RenderTool2D(Renderer2DT& Renderer) const
{
    if (!IsActiveTool()) return;

    // Render the trafo box around the selection.
    m_TrafoBox.Render(Renderer, Options.colors.ToolSelection, Options.colors.ToolHandle);


    switch (m_ToolState)
    {
        case TS_IDLE:
        {
            break;
        }

        case TS_UNDECIDED:
        {
            break;
        }

        case TS_POINT_SEL:
        {
            break;
        }

        case TS_DRAG_SEL:
        {
            const BoundingBox3fT DragBB(m_LDownPosWorld, m_LDragPosWorld);
            const ViewWindow2DT& ViewWin=Renderer.GetViewWin2D();

            Renderer.SetLineType(wxPENSTYLE_DOT, Renderer2DT::LINE_THIN, wxColor(0, 255, 255));
            Renderer.Rectangle(wxRect(ViewWin.WorldToTool(DragBB.Min), ViewWin.WorldToTool(DragBB.Max)), false);
            break;
        }

        case TS_BOX_TRAFO:
        {
            // Draw a preview of the selected objects ONLY if they're currently being modified (translated, scaled, rotated or sheared).
            // Objects that are selected but resting need not be drawn here - such objects already render themselves properly.
            wxASSERT(m_TrafoBox.GetDragState() != TrafoBoxT::TH_NONE);  // A requirement for calling TrafoBoxT::ApplyTrafo().
            if (m_TrafoBox.GetDragState() == TrafoBoxT::TH_NONE) return;

            // If too many individual elements are selected, do not draw them, for performance reasons.
            // The user will then only see the transformed box rectangle as rendered above.
            if (m_MapDoc.GetSelection().Size() > 64) return;

            for (unsigned int SelNr = 0; SelNr < m_MapDoc.GetSelection().Size(); SelNr++)
            {
                MapElementT*   Elem = m_MapDoc.GetSelection()[SelNr];
                TrafoMementoT* Mem  = Elem->GetTrafoState();

                m_TrafoBox.ApplyTrafo(Elem, Options.general.LockingTextures);
                Elem->Render2D(Renderer);

                Elem->RestoreTrafoState(Mem);
                delete Mem;
            }

            break;
        }
    }
}


void ToolSelectionT::RenderTool3D(Renderer3DT& Renderer) const
{
    if (!IsActiveTool()) return;

    m_TrafoBox.Render(Renderer, Options.colors.ToolSelection, Options.colors.ToolHandle);
}


bool ToolSelectionT::UpdateStatusBar(ChildFrameT* ChildFrame) const
{
    return m_TrafoBox.UpdateStatusBar(ChildFrame);
}


void ToolSelectionT::NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection)
{
    if (!IsActiveTool()) return;

    // Update our transformation box according to the new selection.
    UpdateTrafoBox();

    // Notify all observers that this tool changed.
    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolSelectionT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities)
{
    // Clean-up the hit list.
    m_HitList.Overwrite();
    m_CurHitNr=-1;
}


void ToolSelectionT::NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives)
{
    // Clean-up the hit list.
    m_HitList.Overwrite();
    m_CurHitNr=-1;
}


void ToolSelectionT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail)
{
    if (!IsActiveTool()) return;
    if (Detail!=MEMD_PRIMITIVE_PROPS_CHANGED && Detail!=MEMD_GENERIC) return;

    // Just assume that the MapElements set overlaps (intersects) the m_MapDoc.GetSelection() set (and thus the *selection* changed).
    // (Most likely, MapElements *is* the selection; in fact, it's highly unlikely that all elements in MapElements are unselected.)

    // Update our transformation box according to the new selection.
    UpdateTrafoBox();

    // Notify all observers that this tool changed.
    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolSelectionT::NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds)
{
    if (!IsActiveTool()) return;
    if (Detail!=MEMD_TRANSFORM && Detail!=MEMD_GENERIC && Detail!=MEMD_PRIMITIVE_PROPS_CHANGED) return;

    // Just assume that the MapElements set overlaps (intersects) the m_MapDoc.GetSelection() set (and thus the *selection* changed).
    // (Most likely, MapElements *is* the selection; in fact, it's highly unlikely that all elements in MapElements are unselected.)

    // Update our transformation box according to the new selection.
    UpdateTrafoBox();

    // Notify all observers that this tool changed.
    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


/// This is called when a property of an entity changed, such as the "model" property.
/// As changes of such properties can have broad effects, such as changes in the bounding-boxes of the affected elements,
/// we update accordingly.
void ToolSelectionT::Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<CompMapEntityT> >& Entities, EntityModDetailE Detail)
{
    if (!IsActiveTool()) return;

    // Just assume that the MapElements set overlaps (intersects) the m_MapDoc.GetSelection() set (and thus the *selection* changed).
    // (Most likely, MapElements *is* the selection; in fact, it's highly unlikely that all elements in MapElements are unselected.)

    // Update our transformation box according to the new selection.
    UpdateTrafoBox();

    // Notify all observers that this tool changed.
    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolSelectionT::Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var)
{
    if (!IsActiveTool()) return;
    if (strcmp(Var.GetName(), "Origin") != 0 && strcmp(Var.GetName(), "Orientation") != 0) return;

    // Just assume that Var is a variable of an entity that is in the selection,
    // because updating ourselves to the current selection is really all that we can do.

    // Update our transformation box according to the new selection.
    UpdateTrafoBox();

    // Notify all observers that this tool changed.
    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolSelectionT::NotifySubjectDies(SubjectT* dyingSubject)
{
    // We should never get here, since the map document always dies after the tool.
    wxASSERT(false);
}


void ToolSelectionT::OnEscape(ViewWindowT& ViewWindow)
{
    // This is a bit like OnLMouseUp2D(), but aborts rather than completes the operations.
    // Most tool states require no special action before we go back to idle state anyway.
    m_CycleHitsTimer.Stop();

    switch (m_ToolState)
    {
        case TS_IDLE:
            break;

        case TS_UNDECIDED:
            break;

        case TS_POINT_SEL:
            break;

        case TS_DRAG_SEL:
            break;

        case TS_BOX_TRAFO:
            m_TrafoBox.FinishTrafo();
            break;
    }

    m_ToolState=TS_IDLE;
    m_ToolMan.UpdateAllObservers(this, UPDATE_SOON);
}


void ToolSelectionT::UpdateTrafoBox()
{
    BoundingBox3fT    NewBB;
    ArrayT<Vector3fT> ExtraRefPos;

    for (unsigned long SelNr = 0; SelNr < m_MapDoc.GetSelection().Size(); SelNr++)
    {
        MapElementT* Elem = m_MapDoc.GetSelection()[SelNr];

        NewBB.InsertValid(Elem->GetBB());

        // Pick up some extra reference points that the m_TrafoBox can use
        // (besides the box corners) for grid snapping in body dragging mode.
        if (Elem->GetType() == &MapEntRepresT::TypeInfo)
        {
            IntrusivePtrT<CompMapEntityT> Entity = Elem->GetParent();

            ExtraRefPos.PushBack(Entity->GetEntity()->GetTransform()->GetOriginWS());
        }
    }

    m_TrafoBox.SetBB(NewBB, ExtraRefPos);
}


void ToolSelectionT::CreateModel(const Vector3fT& WorldPos)
{
    // TODO Can we choose the scale factor in a way to fit the model into the height of the bounding box?
    wxFileName ModelFile(wxFileSelector("Select a model file", m_MapDoc.GetGameConfig()->ModDir+"/Models/", "", "", "All Files (*.*)|*.*|Model files (*.mdl)|*.mdl|Model Files (*.ase)|*.ase|Model Files (*.dlod)|*.dlod", wxFD_OPEN | wxFD_FILE_MUST_EXIST));

    if (ModelFile=="") return;

    ModelFile.MakeRelativeTo(m_MapDoc.GetGameConfig()->ModDir);
    MapModelT* NewModel=new MapModelT(m_MapDoc, ModelFile.GetFullPath(wxPATH_UNIX), WorldPos);

    m_MapDoc.CompatSubmitCommand(new CommandAddPrimT(m_MapDoc, NewModel, m_MapDoc.GetRootMapEntity(), "new model"));
}


void ToolSelectionT::CreatePlant(const Vector3fT& WorldPos)
{
    wxFileName PlantDescrFile(wxFileSelector("Select a plant description", m_MapDoc.GetGameConfig()->ModDir+"/Plants/", "", "", "Plant Descriptions (*.cpd)|*.cpd|All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST));

    if (PlantDescrFile=="") return;

    PlantDescrFile.MakeRelativeTo(m_MapDoc.GetGameConfig()->ModDir);

    PlantDescriptionT* PlantDescr=m_MapDoc.GetPlantDescrMan().GetPlantDescription(std::string(PlantDescrFile.GetFullPath(wxPATH_UNIX)));
    MapPlantT*         NewPlant  =new MapPlantT(PlantDescr, PlantDescr->RandomSeed, WorldPos);

    m_MapDoc.CompatSubmitCommand(new CommandAddPrimT(m_MapDoc, NewPlant, m_MapDoc.GetRootMapEntity(), "new plant"));
}


void ToolSelectionT::NudgeSelection(const AxesInfoT& AxesInfo, const wxKeyEvent& KE)
{
    const float Dist=(KE.AltDown()!=m_MapDoc.IsSnapEnabled()) ? m_MapDoc.GetGridSpacing() : 1.0f;   // "!=" implements "xor" for bools.
    Vector3fT   NudgeVec;

    switch (KE.GetKeyCode())
    {
        case WXK_RIGHT: NudgeVec[AxesInfo.HorzAxis]=AxesInfo.MirrorHorz ? -Dist :  Dist; break;
        case WXK_LEFT:  NudgeVec[AxesInfo.HorzAxis]=AxesInfo.MirrorHorz ?  Dist : -Dist; break;
        case WXK_DOWN:  NudgeVec[AxesInfo.VertAxis]=AxesInfo.MirrorVert ? -Dist :  Dist; break;
        case WXK_UP:    NudgeVec[AxesInfo.VertAxis]=AxesInfo.MirrorVert ?  Dist : -Dist; break;
    }

    m_MapDoc.CompatSubmitCommand(
        new CommandTransformT(m_MapDoc, m_MapDoc.GetSelection(), CommandTransformT::MODE_TRANSLATE, Vector3fT(), NudgeVec, Options.general.LockingTextures));
}


void ToolSelectionT::SetHitList(const ArrayT<MapElementT*>& NewHits, bool IsControlDown)
{
    // Clear the selection if the user isn't holding down the control (CTRL) key.
    if (!IsControlDown)
    {
        m_MapDoc.GetChildFrame()->GetSurfacePropsDialog()->ClearSelection();
        m_MapDoc.CompatSubmitCommand(CommandSelectT::Clear(&m_MapDoc));
    }

    // Set the new hit list.
    m_HitList=NewHits;
    m_CurHitNr=-1;

    // If the hit list has elements, start the cycle sequence.
    if (m_HitList.Size()>0)
    {
        m_CycleHitsTimer.Notify();
        m_CycleHitsTimer.Start(500);
    }
}


void ToolSelectionT::StepCurHitNr(int Step)
{
    if (m_HitList.Size()==0) return;

    // Toggle the selection state of the previously current m_HitList element (unless it is locked).
    ToggleCurHitNr();

    // Advance m_CurHitNr according to Step.
    m_CurHitNr+=Step;

    if (m_CurHitNr<0) m_CurHitNr=m_HitList.Size()-1;
    if (m_CurHitNr>=int(m_HitList.Size())) m_CurHitNr=0;

    // Toggle the selection state of the newly current m_HitList element (unless it is locked).
    ToggleCurHitNr();
}


void ToolSelectionT::ToggleCurHitNr()
{
    // If the m_CurHitNr is invalid (e.g. -1), skip.
    if (m_CurHitNr<0) return;
    if (m_CurHitNr>=int(m_HitList.Size())) return;

    MapElementT* Elem=m_HitList[m_CurHitNr];

    // If Elem is locked ("cannot select"), skip.
    // Being able to also keep locked elements in the m_HitList is an important feature for 3D view selections.
    // Otherwise we had to filter locked elements out before calling SetHitList(), which subtly changes the
    // tools behaviour that in turn might give the user the impression that the tool is buggy.
    if (!Elem->CanSelect()) return;


    // Implement the toggle.
    // Remember that with the Control key, the m_HitList can have map elements with any selection state.
    ArrayT<MapElementT*> RemoveFromSel;
    ArrayT<MapElementT*> AddToSel;

    // Compute the consequences of toggling the element.
    Elem->GetToggleEffects(RemoveFromSel, AddToSel, m_MapDoc.GetAutoGroupEntities());

    if (RemoveFromSel.Size() > 0) m_MapDoc.CompatSubmitCommand(CommandSelectT::Remove(&m_MapDoc, RemoveFromSel));
    if (     AddToSel.Size() > 0) m_MapDoc.CompatSubmitCommand(CommandSelectT::   Add(&m_MapDoc,      AddToSel));
}


/// This method creates a command that implements the clone-drag operation.
///
/// The main difficulty is that the selection (the source set of elements) can be a completely arbitrary subset
/// of the world's entity hierarchy, and we still must get all hierarchical relationships and all transforms right.
/// For example, consider an entity hierarchy like this:
///
///     ...---A---B---C---D
///
/// and assume that B and D are among the source elements, but C is not. Observe how that makes it very difficult
/// to copy D, as its transform is defined in parent-space, that is, relative to entity C.
/// In order to deal with the problem, we could
///
///   a) directly attach the copy of D to the copy of B, and either ignore that the world-space transform of D's
///      copy will likely turn out wrong, or somehow "fix" the parent-space transform of D's copy to account for
///      a lack of a copy of C,
///
///   b) figure out that C is a gap and forcibly copy it as well (but not necessarily any siblings of C),
///
///   c) copy the whole subtree rooted at B.
///
/// It turns out that both a) and b) are surprisingly difficult to implement! We have therefore opted for c), not
/// only because it is relatively simple to implement, but also for consistency with the rest of the Map Editor:
/// In the "Edit" menu, the "Cut (Ctrl+X)" operation involves a "Delete", and thus *requires* c)-like behaviour.
/// Consequently, "Copy (Ctrl+C)" should work analogously, and using the same behaviour here seems only consistent.
CommandT* ToolSelectionT::CloneDrag() const
{
    // Reduce, split, clone, attach
    // ****************************

    ArrayT<MapElementT*> SourceElems = m_MapDoc.GetSelection();

    MapDocumentT::Reduce(SourceElems);

    ArrayT< IntrusivePtrT<cf::GameSys::EntityT> > NewEnts;
    ArrayT<MapPrimitiveT*>                        NewPrims;

    // Split the reduced set of SourceElems into entities and primitives, clone them (fully recursively), and attach
    // each clone to its respective parent. The last step is necessary because the transforms below work in world-
    // space, and the world-space transform of an entity can only be determined while it is properly anchored in the
    // world. (This is contrary to the primitives, which are always in world-space anyway.)
    for (unsigned int ElemNr = 0; ElemNr < SourceElems.Size(); ElemNr++)
    {
        MapElementT* Elem = SourceElems[ElemNr];

        if (Elem->GetType() == &MapEntRepresT::TypeInfo)
        {
            // Double-check that this is really a MapEntRepresT.
            wxASSERT(Elem->GetParent()->GetRepres() == Elem);

            IntrusivePtrT<cf::GameSys::EntityT> OldEnt = Elem->GetParent()->GetEntity();
            IntrusivePtrT<cf::GameSys::EntityT> NewEnt = new cf::GameSys::EntityT(*OldEnt, true /*Recursive*/);

            GetMapEnt(NewEnt)->CopyPrimitives(*GetMapEnt(OldEnt), true /*Recursive*/);

            NewEnts.PushBack(NewEnt);

            if (OldEnt->GetParent() != NULL)
                OldEnt->GetParent()->AddChild(NewEnt);
            else
                OldEnt->AddChild(NewEnt);   // Clone-dragging the world!
        }
        else
        {
            // Double-check that this is really *not* a MapEntRepresT.
            wxASSERT(Elem->GetParent()->GetRepres() != Elem);

            MapPrimitiveT* OldPrim = dynamic_cast<MapPrimitiveT*>(Elem);
            MapPrimitiveT* NewPrim = OldPrim->Clone();

            NewPrims.PushBack(NewPrim);

            OldPrim->GetParent()->AddPrim(NewPrim);
        }
    }


    // Transform all elements
    // **********************

    ArrayT<MapElementT*> AllNewElems;

    for (unsigned int EntNr = 0; EntNr < NewEnts.Size(); EntNr++)
    {
        ArrayT<MapElementT*> Elems = GetMapEnt(NewEnts[EntNr])->GetAllMapElements();

        for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
        {
            m_TrafoBox.ApplyTrafo(Elems[ElemNr], Options.general.LockingTextures);
            AllNewElems.PushBack(Elems[ElemNr]);
        }
    }

    for (unsigned int PrimNr = 0; PrimNr < NewPrims.Size(); PrimNr++)
    {
        m_TrafoBox.ApplyTrafo(NewPrims[PrimNr], Options.general.LockingTextures);
        AllNewElems.PushBack(NewPrims[PrimNr]);
    }


    // Detach from parents again, then re-attach with commands
    // *******************************************************

    ArrayT<CommandT*> SubCommands;

    for (unsigned int EntNr = 0; EntNr < NewEnts.Size(); EntNr++)
    {
        IntrusivePtrT<cf::GameSys::EntityT> Parent = NewEnts[EntNr]->GetParent();

        Parent->RemoveChild(NewEnts[EntNr]);
        SubCommands.PushBack(new CommandNewEntityT(m_MapDoc, NewEnts[EntNr], Parent, false /*don't select*/));
    }

    for (unsigned int PrimNr = 0; PrimNr < NewPrims.Size(); PrimNr++)
    {
        IntrusivePtrT<CompMapEntityT> Parent = NewPrims[PrimNr]->GetParent();

        Parent->RemovePrim(NewPrims[PrimNr]);
        SubCommands.PushBack(new CommandAddPrimT(m_MapDoc, NewPrims[PrimNr], Parent, "add prims", false /*don't select*/));
    }

    SubCommands.PushBack(CommandSelectT::Set(&m_MapDoc, AllNewElems));

    return new CommandMacroT(SubCommands, "Clone");
}
