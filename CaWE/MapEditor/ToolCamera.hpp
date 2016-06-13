/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_CAMERA_HPP_INCLUDED
#define CAFU_TOOL_CAMERA_HPP_INCLUDED

#include "Tool.hpp"
#include "Math3D/Vector3.hpp"
#include "Templates/Array.hpp"


class CameraT;
class OptionsBar_CameraToolT;
class ViewWindowT;


class ToolCameraT : public ToolT
{
    public:

    ToolCameraT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);
    ~ToolCameraT();

    const ArrayT<CameraT*>& GetCameras() const { return m_Cameras; }
    CameraT* GetActiveCamera() const { return m_Cameras[m_ActiveCameraNr]; }
    void AddCamera(CameraT* Camera);
    void DeleteActiveCamera();
    void NotifyCameraChanged(const CameraT* Camera);


    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_CAMERA; }
    wxWindow* GetOptionsBar();

    bool OnKeyDown2D   (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnLMouseUp2D  (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove2D (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);

    bool OnKeyDown3D   (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);
    bool OnMouseMove3D (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);

    void RenderTool2D(Renderer2DT& Renderer) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    enum CameraPartT
    {
        POSITION,
        LOOKAT
    };

    enum ToolStateT
    {
        IDLE,
        DRAG_CAM_POS,
        DRAG_CAM_DIR
    };


    /// Determines if in ViewWindow at PointWS (in window space) is one of our cameras (i.e. the position or look-at point of one of our cameras).
    bool FindCamera(ViewWindow2DT& ViewWindow, const wxPoint& PointWS, unsigned long& CamNr, CameraPartT& CamPart) const;

    /// Handles key down events that are common to the 2D and 3D views.
    bool OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE);


    ArrayT<CameraT*> m_Cameras;         ///< The cameras that have been created, there is always at least one.
    unsigned long    m_ActiveCameraNr;  ///< The index number of the camera that is currently active.
    ToolStateT       m_ToolState;       ///< The state this tool is currently in.

    OptionsBar_CameraToolT* m_OptionsBar;   ///< The options bar for this tool.
};

#endif
