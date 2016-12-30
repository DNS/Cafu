/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ChildFrameViewWin3D.hpp"
#include "ChildFrame.hpp"
#include "MapDocument.hpp"
#include "MapPrimitive.hpp"
#include "ToolManager.hpp"
#include "ToolCamera.hpp"

#include "../AppCaWE.hpp"
#include "../Camera.hpp"
#include "../Options.hpp"
#include "../ParentFrame.hpp"

#include "GameSys/Entity.hpp"
#include "GameSys/World.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

#include "wx/wx.h"


#if defined(_WIN32) && defined(_MSC_VER)
// Turn off warning 4355: "'this' : wird in Initialisierungslisten fuer Basisklasse verwendet".
#pragma warning(disable:4355)
#endif


BEGIN_EVENT_TABLE(ViewWindow3DT, Generic3DWindowT)
    EVT_KEY_DOWN     (ViewWindow3DT::OnKeyDown        )
    EVT_KEY_UP       (ViewWindow3DT::OnKeyUp          )
    EVT_CHAR         (ViewWindow3DT::OnKeyChar        )
    EVT_LEFT_DOWN    (ViewWindow3DT::OnMouseLeftDown  )
    EVT_LEFT_DCLICK  (ViewWindow3DT::OnMouseLeftDown  )
    EVT_LEFT_UP      (ViewWindow3DT::OnMouseLeftUp    )
    EVT_MIDDLE_DOWN  (ViewWindow3DT::OnMouseMiddleDown)
    EVT_MIDDLE_DCLICK(ViewWindow3DT::OnMouseMiddleDown)
    EVT_MIDDLE_UP    (ViewWindow3DT::OnMouseMiddleUp  )
    EVT_MOUSEWHEEL   (ViewWindow3DT::OnMouseWheel     )
    EVT_MOTION       (ViewWindow3DT::OnMouseMove      )
    EVT_CONTEXT_MENU (ViewWindow3DT::OnContextMenu    )
    EVT_PAINT        (ViewWindow3DT::OnPaint          )
END_EVENT_TABLE()


static ToolCameraT* GetCameraTool(ChildFrameT* ChildFrame)
{
    return static_cast<ToolCameraT*>(ChildFrame->GetToolManager().GetTool(ToolCameraT::TypeInfo));
}


ViewWindow3DT::ViewWindow3DT(wxWindow* Parent, ChildFrameT* ChildFrame, CameraT* InitialCamera, ViewTypeT InitialViewType)
    : Generic3DWindowT(Parent, InitialCamera ? InitialCamera : GetCameraTool(ChildFrame)->GetCameras()[0]),
      ViewWindowT(ChildFrame),
      m_ViewType(InitialViewType),
      m_Renderer(*this),
      m_TimeOfLastPaint(0),
      m_CameraTool(GetCameraTool(ChildFrame))
{
    // Make sure that if we were given an InitialCamera for use, it is known to the Camera tool
    // (the AddCamera() method in turn makes sure that InitialCamera is not added twice).
    if (InitialCamera) m_CameraTool->AddCamera(InitialCamera);

    // The initial view type was read from a config file - make sure that it is valid.
    if (m_ViewType<VT_3D_WIREFRAME) m_ViewType=VT_3D_WIREFRAME;
}


void ViewWindow3DT::NotifySubjectChanged(ToolsSubjectT* Subject, ToolT* Tool, ToolsUpdatePriorityT Priority)
{
    wxASSERT((Tool->GetType()==&ToolCameraT::TypeInfo)==(Tool==m_CameraTool));

    if (Tool==m_CameraTool && m_CameraTool->GetActiveCamera()!=&GetCamera())
    {
        // A camera has changed, but its not the one we use for this view.
        // So don't waste performance with updating when nothing has changed.
        return;
    }

    switch (Priority)
    {
        case UPDATE_NOW:
            Refresh(false /*eraseBackground*/);
            Update();
            break;

        case UPDATE_SOON:
            // No need to call Refresh(false) here as well,
            // just wait for the next idle event to trigger the repaint.
            break;
    }
}


wxWindow* ViewWindow3DT::GetWindow()
{
    return this;
}


static bool CompareHitDepths(const ViewWindow3DT::HitInfoT& Hit1, const ViewWindow3DT::HitInfoT& Hit2)
{
    return Hit1.Depth<Hit2.Depth;
}


ArrayT<ViewWindow3DT::HitInfoT> ViewWindow3DT::GetElementsAt(const wxPoint& Pixel) const
{
    wxASSERT(&GetMapDoc());     // Can be NULL between Destroy() and the dtor, but between those we should never get here.

    ArrayT<ViewWindow3DT::HitInfoT> Hits;

    // Note that our ray does intentionally not start at GetCamera().Pos,
    // but at the point of intersection with the near clipping plane!
    const Vector3fT RayOrigin = WindowToWorld(Pixel);
    const Vector3fT RayDir    = normalizeOr0(RayOrigin - GetCamera().Pos);

    // Make sure that the ray is valid. It should never be invalid though.
    if (length(RayDir) < 0.9f) return Hits;

    // This just iterates over all elements in the world (including the `MapEntRepresT` instances) in a brute force manner.
    // It would certainly be nice to optimize this in some way, e.g. by making use of the BSP tree,
    // but our Ray-AABB intersection tests are really fast, so this is OK for now.
    ArrayT<MapElementT*> Elems;
    GetMapDoc().GetAllElems(Elems);

    for (unsigned int ElemNr = 0; ElemNr < Elems.Size(); ElemNr++)
    {
        MapElementT*   Elem     = Elems[ElemNr];
        float          Fraction = 0.0f;
        unsigned long  FaceNr   = 0;

        if (Elem->IsVisible() && Elem->TraceRay(RayOrigin, RayDir, Fraction, FaceNr))
        {
            ViewWindow3DT::HitInfoT Hit;

            Hit.Object = Elem;
            Hit.FaceNr = FaceNr;
            Hit.Depth  = Fraction;
            Hit.Pos    = RayOrigin + RayDir*Fraction;

            Hits.PushBack(Hit);
        }
    }

    Hits.QuickSort(CompareHitDepths);

    return Hits;
}


Vector3fT ViewWindow3DT::GetRefPtWorld(const wxPoint& RefPtWin) const
{
    const ArrayT<HitInfoT> Hits=GetElementsAt(RefPtWin);

    return Hits.Size()>0 ? Hits[0].Pos : GetCamera().Pos;
}


void ViewWindow3DT::InfoCameraChanged()
{
    m_CameraTool->NotifyCameraChanged(&GetCamera());
}


// As the RMB serves multiple functions (click, context-menu, camera control),
// the Generic3DWindowT keeps tight control over it and thus its event are handled differently.
void ViewWindow3DT::InfoRightMouseClick(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) return;

    // Give the active tool a chance to intercept the event.
    ToolT*     Tool   =m_ChildFrame->GetToolManager().GetActiveTool();
    const bool Handled=Tool && Tool->OnRMouseClick3D(*this, ME);

    if (!Handled)
    {
        // The tool did not handle the event, now show the context menu.
        // Showing the context menu manually here also makes RMB handling uniform
        // across platforms, see <http://trac.wxwidgets.org/ticket/12535> for details.
        wxContextMenuEvent CE(wxEVT_CONTEXT_MENU, GetId(), ClientToScreen(ME.GetPosition()));

        // We don't inline the OnContextMenu() code here, because context menus can also be opened via the keyboard.
        CE.SetEventObject(this);
        OnContextMenu(CE);
    }
}


void ViewWindow3DT::OnKeyDown(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnKeyDown3D(*this, KE)) return;

    switch (KE.GetKeyCode())
    {
        case WXK_TAB:
        {
            // Cycle the view to the next or previous 3D view type.
            // Note that checking for KE.ControlDown() here is meaningless,
            // because the Control+TAB combination is already caught by the MDI framework.
            m_ViewType=ViewTypeT(m_ViewType+(KE.ShiftDown() ? -1 : 1));

            if (m_ViewType<VT_3D_WIREFRAME) m_ViewType=VT_3D_FULL_MATS;
            if (m_ViewType>VT_3D_FULL_MATS) m_ViewType=VT_3D_WIREFRAME;

            m_ChildFrame->SetCaption(this, GetCaption());
            break;
        }

        case WXK_PAGEDOWN:
        {
            const int CamNr=m_CameraTool->GetCameras().Find(const_cast<CameraT*>(&GetCamera()));

            wxASSERT(CamNr!=-1);

            SetCamera(m_CameraTool->GetCameras()[CamNr+1>=int(m_CameraTool->GetCameras().Size()) ? 0 : CamNr+1]);
            m_CameraTool->NotifyCameraChanged(&GetCamera());
            break;
        }

        case WXK_PAGEUP:
        {
            const int CamNr=m_CameraTool->GetCameras().Find(const_cast<CameraT*>(&GetCamera()));

            wxASSERT(CamNr!=-1);

            SetCamera(m_CameraTool->GetCameras()[CamNr<=0 ? m_CameraTool->GetCameras().Size()-1 : CamNr-1]);
            m_CameraTool->NotifyCameraChanged(&GetCamera());
            break;
        }

        default:
            // Event not handled here - now let the base class process it.
            KE.Skip();
            break;
    }
}


void ViewWindow3DT::OnKeyUp(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnKeyUp3D(*this, KE)) return;

    // Event not handled here - now let the base class process it.
    KE.Skip();
}


void ViewWindow3DT::OnKeyChar(wxKeyEvent& KE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { KE.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnChar3D(*this, KE)) return;

    // Event not handled here - now let the base class process it.
    KE.Skip();
}


/// Note that we setup the wxWidgets event table so that this method also handles the "double-click" event,
/// because when a double-click occurs, the second mouse down event is otherwise "replaced" by a double-click event.
/// That is, the normal event sequence with a double-click is always as follows:
///    1. mouse button down
///    2. mouse button up
///    3. mouse button double-click (instead of "down")
///    4. mouse button up
/// Therefore, in order to keep the handling simple, we set the event table to send double-clicks also to this method.
/// This is especially very important with capturing the mouse on down events and releasing it on up events!
/// It is still possible to learn whether the event was a normal "down" or a "double-click" event by calling ME.ButtonDClick() for distinction.
void ViewWindow3DT::OnMouseLeftDown(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnLMouseDown3D(*this, ME)) return;

    ME.Skip();
}


void ViewWindow3DT::OnMouseLeftUp(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnLMouseUp3D(*this, ME)) return;

    ME.Skip();
}


void ViewWindow3DT::OnMouseMiddleDown(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnMMouseDown3D(*this, ME)) return;

    // Event not handled here - now let the base class process it.
    ME.Skip();
}


void ViewWindow3DT::OnMouseMiddleUp(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnMMouseUp3D(*this, ME)) return;

    // Event not handled here - now let the base class process it.
    ME.Skip();
}


void ViewWindow3DT::OnMouseWheel(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Give the active tool a chance to intercept the event.
    ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
    if (Tool && Tool->OnMouseWheel3D(*this, ME)) return;

    // Event not handled here - now let the base class process it.
    ME.Skip();
}


void ViewWindow3DT::OnMouseMove(wxMouseEvent& ME)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { ME.Skip(); return; }

    // Ok, there is mouse movement in this view, so update its MRU position in the child frames list.
    UpdateChildFrameMRU();

    // Moving the mouse over one of the 2D or 3D views also assigns the keyboard input focus to it.
    // This saves the user from having to explicitly LMB-click into it, which might have undesired side-effects depending
    // on the currently active tool. However, note that we only want to do this if we're currently the foreground application.
    if (wxGetApp().IsActive())
        if (wxWindow::FindFocus()!=this) SetFocus();

    // Update the status bar info.
    wxASSERT(m_ChildFrame);     // See the description of NotifyChildFrameDies() for why this should never trigger.
    m_ChildFrame->SetStatusText("", ChildFrameT::SBP_GRID_ZOOM);
    m_ChildFrame->SetStatusText("", ChildFrameT::SBP_MOUSE_POS);

    // Make sure that the base class always gets this event as well.
    ME.Skip();

    if (!GetMouseControl().IsActive())
    {
        // Give the active tool a chance to intercept the event.
        ToolT* Tool=m_ChildFrame->GetToolManager().GetActiveTool();
        if (Tool && Tool->OnMouseMove3D(*this, ME)) return;
    }
}


void ViewWindow3DT::OnContextMenu(wxContextMenuEvent& CE)
{
    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { CE.Skip(); return; }

    wxMenu Menu;

    Menu.AppendRadioItem(VT_3D_WIREFRAME,  "3D Wireframe" );
    Menu.AppendRadioItem(VT_3D_FLAT,       "3D Flat"      );
    Menu.AppendRadioItem(VT_3D_EDIT_MATS,  "3D Edit Mats" );
    Menu.AppendRadioItem(VT_3D_FULL_MATS,  "3D Full Mats" );
    Menu.AppendRadioItem(VT_3D_LM_GRID,    "3D LM Grid"   );
    Menu.AppendRadioItem(VT_3D_LM_PREVIEW, "3D LM Preview");
    Menu.Check(GetViewType(), true);

    ToolT*    Tool     =m_ChildFrame->GetToolManager().GetActiveTool();
    const int MenuSelID=Tool ? Tool->OnContextMenu3D(*this, CE, Menu) : GetPopupMenuSelectionFromUser(Menu);

    if (MenuSelID>=VT_3D_WIREFRAME && MenuSelID<=VT_3D_LM_PREVIEW)
    {
        m_ViewType=ViewTypeT(MenuSelID);

        m_ChildFrame->SetCaption(this, GetCaption());
    }
}


void ViewWindow3DT::OnPaint(wxPaintEvent& PE)
{
    MapDocumentT& MapDoc=GetMapDoc();

    // Guard against accessing an already deleted MapDoc. This can otherwise happen when closing this window/view/document,
    // namely during the continued event processing between the call to Destroy() and our final deletion.
    if (&GetMapDoc()==NULL) { PE.Skip(); return; }


    // Determine how much time has passed since the previous frame.
    if (m_TimeOfLastPaint==0) m_TimeOfLastPaint=wxGetLocalTimeMillis().GetLo();

    const unsigned long TimeNow  =wxGetLocalTimeMillis().GetLo();
    const float         FrameTime=std::min(float(TimeNow-m_TimeOfLastPaint)/1000.0f, 0.5f);

    m_TimeOfLastPaint=TimeNow;

    if (Options.view3d.AnimateModels)
        MapDoc.GetScriptWorld().OnClientFrame(FrameTime);


    /*********************/
    /*** Process Input ***/
    /*********************/

    ProcessInput(FrameTime);


    /********************/
    /*** Render Frame ***/
    /********************/

    wxPaintDC dc(this);     // It is VERY important not to omit this, or otherwise everything goes havoc.

    if (!wxGetApp().IsActive()) return;


    // ********************
    // Pre-rendering stuff.
    // ********************

    m_Renderer.InitFrame();

    // Initialize the viewport.
    SetCurrent(*wxGetApp().GetParentFrame()->m_GLContext);    // This is the method from the wxGLCanvas for activating the given RC with this window.
    const wxSize   CanvasSize=GetClientSize();
    const CameraT& Camera    =GetCamera();

    MatSys::Renderer->SetViewport(0, 0, CanvasSize.GetWidth(), CanvasSize.GetHeight());

    // Initialize the perspective projection (view-to-clip) matrix.
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION,
        MatrixT::GetProjPerspectiveMatrix(Camera.VerticalFOV, (float)CanvasSize.GetWidth()/(float)CanvasSize.GetHeight(),
                                          Camera.NearPlaneDist, Camera.FarPlaneDist));

    // Initialize the camera (world-to-view) matrix.
    MatrixT WorldToView=Camera.GetMatrix();

    // Rotate by 90 degrees around the x-axis in order to meet the MatSys's expectation of axes orientation.
    for (unsigned long i=0; i<4; i++)
    {
        std::swap(WorldToView[1][i], WorldToView[2][i]);
        WorldToView[2][i]=-WorldToView[2][i];
    }

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, WorldToView);
    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentLightMap(wxGetApp().GetParentFrame()->m_WhiteTexture);    // Set a proper default lightmap.
    MatSys::Renderer->SetCurrentLightDirMap(NULL);      // The MatSys provides a default for LightDirMaps when NULL is set.
    MatSys::Renderer->ClearColor(0, 0, 0, 0);
    MatSys::Renderer->BeginFrame(TimeNow/1000.0);


    // ************************
    // Do the actual rendering.
    // ************************

    // Render the world axes. They're great for technical and emotional reassurance.
    {
        static MatSys::MeshT Mesh(MatSys::MeshT::Lines);

        if (Mesh.Vertices.Size()==0) Mesh.Vertices.PushBackEmpty(6);

        Mesh.Vertices[0].SetColor(1, 0, 0); Mesh.Vertices[0].SetOrigin(0, 0, 0);
        Mesh.Vertices[1].SetColor(1, 0, 0); Mesh.Vertices[1].SetOrigin(100, 0, 0);
        Mesh.Vertices[2].SetColor(0, 1, 0); Mesh.Vertices[2].SetOrigin(0, 0, 0);
        Mesh.Vertices[3].SetColor(0, 1, 0); Mesh.Vertices[3].SetOrigin(0, 100, 0);
        Mesh.Vertices[4].SetColor(0, 0, 1); Mesh.Vertices[4].SetOrigin(0, 0, 0);
        Mesh.Vertices[5].SetColor(0, 0, 1); Mesh.Vertices[5].SetOrigin(0, 0, 100);

        MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
        MatSys::Renderer->RenderMesh(Mesh);
    }


    // Get the back-to-front ordered list of map elements that are in the view frustum and visible.
    const ArrayT<MapElementT*>& VisElemsBackToFront=m_Renderer.GetVisElemsBackToFront();

    // Render the opaque elements front-to-back.
    for (unsigned long ElemNr=VisElemsBackToFront.Size(); ElemNr>0; ElemNr--)
        if (!VisElemsBackToFront[ElemNr-1]->IsTranslucent())
            VisElemsBackToFront[ElemNr-1]->Render3D(m_Renderer);

    // Render the translucent elements back-to-front.
    for (unsigned long ElemNr=0; ElemNr<VisElemsBackToFront.Size(); ElemNr++)
        if (VisElemsBackToFront[ElemNr]->IsTranslucent())
            VisElemsBackToFront[ElemNr]->Render3D(m_Renderer);


    // Render the tools.
    const ArrayT<ToolT*>& Tools=m_ChildFrame->GetToolManager().GetTools();

    for (unsigned long ToolNr=0; ToolNr<Tools.Size(); ToolNr++)
        Tools[ToolNr]->RenderTool3D(m_Renderer);


    // Render the "zigzag" line that has been loaded from a point file and usually leads from an entity to a leak
    // or is the description of a players path through a game map as recorded with the recordPath() console function.
    const ArrayT<PtsPointT>& Points=GetMapDoc().GetPointFilePoints();
    const ArrayT<wxColour>&  Colors=GetMapDoc().GetPointFileColors();

    if (Points.Size()>0 && Colors.Size()>=4)
    {
        MatSys::MeshT Mesh(MatSys::MeshT::LineStrip);

        if (Colors[1].IsOk())
        {
            for (unsigned long PointNr=0; PointNr<Points.Size(); PointNr++)
            {
                Mesh.Vertices.PushBackEmpty();

                const Vector3fT&        Pos   =Points[PointNr].Pos;
                MatSys::MeshT::VertexT& Vertex=Mesh.Vertices[Mesh.Vertices.Size()-1];

                Vertex.SetOrigin(Pos.x, Pos.y, Pos.z);
                Vertex.SetColor(Colors[1].Red()/255.0f, Colors[1].Green()/255.0f, Colors[1].Blue()/255.0f);
            }

            MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
            MatSys::Renderer->RenderMesh(Mesh);
        }

        if (Colors[2].IsOk())
        {
            Mesh.Type=MatSys::MeshT::Lines;
            Mesh.Vertices.Overwrite();

            for (unsigned long PointNr=0; PointNr<Points.Size(); PointNr++)
            {
                Mesh.Vertices.PushBackEmpty(2);

                const float             Hdg    =Points[PointNr].Heading/32768.0f*180.0f;
                const Vector3fT&        Pos1   =Points[PointNr].Pos;
                const Vector3fT         Pos2   =Points[PointNr].Pos+Vector3fT(0.0f, 8.0f, 0.0f).GetRotZ(-Hdg);
                MatSys::MeshT::VertexT& Vertex1=Mesh.Vertices[Mesh.Vertices.Size()-2];
                MatSys::MeshT::VertexT& Vertex2=Mesh.Vertices[Mesh.Vertices.Size()-1];

                Vertex1.SetOrigin(Pos1.x, Pos1.y, Pos1.z);
                Vertex1.SetColor(Colors[2].Red()/255.0f, Colors[2].Green()/255.0f, Colors[2].Blue()/255.0f);

                Vertex2.SetOrigin(Pos2.x, Pos2.y, Pos2.z);
                Vertex2.SetColor(Colors[2].Red()/255.0f, Colors[2].Green()/255.0f, Colors[2].Blue()/255.0f);
            }

            MatSys::Renderer->SetCurrentMaterial(m_Renderer.GetRMatWireframe());
            MatSys::Renderer->RenderMesh(Mesh);
        }
    }


    // Render the split planes of the BSP tree (for debugging).
    m_Renderer.RenderSplitPlanes(MapDoc.GetBspTree()->GetRootNode(), Options.view3d.SplitPlanesDepth);


    // Render the "HUD" of the mouse control.
    if (GetMouseControl().IsActive())
    {
        Renderer3DT::UseOrthoMatricesT UseOrtho(*this);

        m_Renderer.RenderCrossHair(GetMouseControl().GetRefPtWin());
    }


    // *********************
    // Post-rendering stuff.
    // *********************

    MatSys::Renderer->EndFrame();
    SwapBuffers();
}
