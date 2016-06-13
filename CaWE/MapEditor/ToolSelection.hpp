/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_SELECTION_HPP_INCLUDED
#define CAFU_TOOL_SELECTION_HPP_INCLUDED

#include "Tool.hpp"
#include "TrafoBox.hpp"
#include "Templates/Array.hpp"


class OptionsBar_SelectionToolT;
class Renderer2DT;
class ViewWindowT;
class ToolSelectionT;


class CycleHitsTimerT : public wxTimer
{
    public:

    CycleHitsTimerT(ToolSelectionT& SelectionTool);

    void Notify();


    private:

    ToolSelectionT& m_SelectionTool;
};


class ToolSelectionT : public ToolT, public ObserverT
{
    public:

    ToolSelectionT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);
    ~ToolSelectionT();

    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_SELECTION; }
    wxWindow* GetOptionsBar();
    void      OnActivate(ToolT* OldTool);
    void      OnDeactivate(ToolT* NewTool);

    bool OnKeyDown2D    (ViewWindow2DT& ViewWindow, wxKeyEvent&         KE);
    bool OnKeyUp2D      (ViewWindow2DT& ViewWindow, wxKeyEvent&         KE);
    bool OnLMouseDown2D (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME);
    bool OnLMouseUp2D   (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME);
    bool OnMouseMove2D  (ViewWindow2DT& ViewWindow, wxMouseEvent&       ME);
    int  OnContextMenu2D(ViewWindow2DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu);

    bool OnKeyDown3D    (ViewWindow3DT& ViewWindow, wxKeyEvent&         KE);
    bool OnLMouseDown3D (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME);
    bool OnLMouseUp3D   (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME);
    bool OnMouseMove3D  (ViewWindow3DT& ViewWindow, wxMouseEvent&       ME);
    int  OnContextMenu3D(ViewWindow3DT& ViewWindow, wxContextMenuEvent& CE, wxMenu& Menu);

    void RenderTool2D(Renderer2DT& Renderer) const;
    void RenderTool3D(Renderer3DT& Renderer) const;
    bool UpdateStatusBar(ChildFrameT* ChildFrame) const;

    // ObserverT implementation.
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT< IntrusivePtrT<cf::GameSys::EntityT> >& Entities);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    void Notify_EntChanged(SubjectT* Subject, const ArrayT< IntrusivePtrT<MapEditor::CompMapEntityT> >& Entities, EntityModDetailE Detail);
    void Notify_VarChanged(SubjectT* Subject, const cf::TypeSys::VarBaseT& Var);
    void NotifySubjectDies(SubjectT* dyingSubject);

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    /// This enumeration defines the essential states of this tool.
    /// Note that in each tool state, the selection can contain any number of map elements.
    enum ToolStateT
    {
        /// The LMB is up and nothing is currently happening.
        /// The mouse cursor however is updated to indicate a likely next state if the button is pressed at the current position.
        TS_IDLE,

        /// The LMB went down in a place where further input is required (e.g. mouse move or button release)
        /// for deciding the action and the next state.
        TS_UNDECIDED,

        /// There was a one-click selection, and the button is still down.
        /// We cycle through subsets of the selected objects (each one separately, and their union) until the button is released.
        TS_POINT_SEL,

        /// The user is dragging a box frame for map element selection.
        TS_DRAG_SEL,

        /// The box around the selected objects is currently being transformed, that is, translated, scaled, rotated or sheared.
        TS_BOX_TRAFO
    };

    /// IDs used for the items in our RMB context menus.
    enum
    {
        ID_CREATE_MODEL=wxID_HIGHEST+1000,
        ID_CREATE_PLANT
    };

    friend class CycleHitsTimerT;

    void OnEscape(ViewWindowT& ViewWindow);     ///< Handles the ESC key event in the 2D and 3D views.
    void UpdateTrafoBox();
    void CreateModel(const Vector3fT& WorldPos);
    void CreatePlant(const Vector3fT& WorldPos);
    void NudgeSelection(const AxesInfoT& AxesInfo, const wxKeyEvent& KE);
    void SetHitList(const ArrayT<MapElementT*>& NewHits, bool IsControlDown);
    void StepCurHitNr(int Step);
    void ToggleCurHitNr();
    CommandT* CloneDrag() const;


    ToolStateT           m_ToolState;       ///< The main state of this tool.
    TrafoBoxT            m_TrafoBox;        ///< The transformation box around the current selection (only dependent on the selection, not the tool state). Depending on its own state, the trafo box can show scale, rotate or shear handles.

    wxPoint              m_LDownPosWin;     ///< The point where the LMB went down, in window coordinates. Set for all tool states.
    Vector3fT            m_LDownPosWorld;   ///< The point where the LMB went down, in world space. Set for all tool states.
    Vector3fT            m_LDragPosWorld;   ///< In state TS_DRAG_SEL, this is the current point in world space of the drag (second corner of the dragging rectangle).

    CycleHitsTimerT      m_CycleHitsTimer;
    ArrayT<MapElementT*> m_HitList;         ///< The list of map elements that were hit by the last LMB click in a 2D or 3D view (along the ray through the pixel).
    int                  m_CurHitNr;        ///< As we're cycling through the m_HitList, this is the number of the currently considered (and selected) list element.

    OptionsBar_SelectionToolT* m_OptionsBar;    ///< The options bar for this tool.
};

#endif
