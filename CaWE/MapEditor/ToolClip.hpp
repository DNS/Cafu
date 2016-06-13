/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_CLIP_HPP_INCLUDED
#define CAFU_TOOL_CLIP_HPP_INCLUDED

#include "Tool.hpp"
#include "Math3D/Plane3.hpp"
#include "Templates/Array.hpp"


struct ClipResultT;
class OptionsBar_ClipBrushesToolT;
class ViewWindowT;


class ToolClipT : public ToolT
{
    public:

    /// The constructor.
    ToolClipT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);

    /// The destructor.
    ~ToolClipT();

    /// This is called by the options bar whenever the clip mode has changed.
    void NoteClipModeChanged();


    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_CLIP; }
    wxWindow* GetOptionsBar();
    void      OnActivate(ToolT* OldTool);
    void      OnDeactivate(ToolT* NewTool);

    bool OnKeyDown2D   (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnLMouseUp2D  (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove2D (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnKeyDown3D   (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);

    void RenderTool2D(Renderer2DT& Renderer) const;
    void RenderTool3D(Renderer3DT& Renderer) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    enum ToolStateT
    {
        IDLE_EMPTY,         ///< The initial state of the tool: The LMB has not yet been clicked, and the two points that define the clip plane are still undefined.
        IDLE_HAVE_POINTS,   ///< The clip points have been set, but currently none of them is being dragged.
        DRAG_POINT_0,       ///< The user is dragging the first  clip point.
        DRAG_POINT_1        ///< The user is dragging the second clip point.
    };


    /// Returns the index of the clip point in ViewWindow at PointWS (in window space), or -1 if none.
    int HitTest(ViewWindow2DT& ViewWindow, const wxPoint& PointWS);

    /// Updates the m_ClipResults, based on the the map documents current selection.
    void UpdateClipResults();

    /// Handles key down events that are common to the 2D and 3D views.
    bool OnKeyDown(ViewWindowT& ViewWindow, wxKeyEvent& KE);


    ToolStateT           m_ToolState;           ///< The state this tool is currently in.
    Vector3fT            m_ClipPoints[2];       ///< The two clip points that define the plane used for clipping the brushes.
    int                  m_Clip3rdAxis;         ///< The third axis of the view in which the m_ClipPoints were defined. This also defines one of the span vectors of the clip plane.
    ArrayT<ClipResultT*> m_ClipResults;         ///< The current results of the clipping operation, updated by UpdateClipResults(). Never touch the Workpiece member, it can be invalid whenever the user had a chance to interfere since the last call to UpdateClipResults(), e.g. by deleting (via the menu) the selected brushes!
    bool                 m_DrawMeasurements;    ///< Whether to draw dimensions of the clipped brushes in the 2D views.

    OptionsBar_ClipBrushesToolT* m_OptionsBar;  ///< The options bar for this tool.
};

#endif
