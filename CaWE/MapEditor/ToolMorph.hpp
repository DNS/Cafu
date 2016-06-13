/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOL_MORPH_HPP_INCLUDED
#define CAFU_TOOL_MORPH_HPP_INCLUDED

#include "Tool.hpp"
#include "ObserverPattern.hpp"


class  AxesInfoT;
class  Renderer2DT;
class  Renderer3DT;
class  MorphPrimT;
class  OptionsBar_EditVerticesToolT;
struct MP_PartT;


struct MorphHandleT
{
    /// The default constructor.
    MorphHandleT() : MorphPrim(NULL), Part(NULL) { }

    MorphPrimT* MorphPrim;
    MP_PartT*   Part;
};


/// This class represents the "Edit Vertices" / "Morph" tool.
class ToolMorphT : public ToolT, public ObserverT
{
    public:

    ToolMorphT(MapDocumentT& MapDoc, ToolManagerT& ToolMan, wxWindow* ParentOptionsBar);

    ~ToolMorphT();


    void NoteEditModeChanged();     ///< This is called by the options bar whenever the edit mode has changed (edit vertices, edges, or both).
    void InsertVertex();            ///< Called by the options bar when the user pressed the "Insert Vertex" button.


    // Implementations/overrides of ToolT methods.
    int       GetWxEventID() const { return ChildFrameT::ID_MENU_TOOLS_TOOL_EDITVERTICES; }
    wxWindow* GetOptionsBar();
    void      OnActivate(ToolT* OldTool);
    void      OnDeactivate(ToolT* NewTool);
    bool      CanDeactivate();

    bool OnKeyDown2D   (ViewWindow2DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown2D(ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove2D (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);
    bool OnLMouseUp2D  (ViewWindow2DT& ViewWindow, wxMouseEvent& ME);

    bool OnKeyDown3D   (ViewWindow3DT& ViewWindow, wxKeyEvent&   KE);
    bool OnLMouseDown3D(ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnMouseMove3D (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);
    bool OnLMouseUp3D  (ViewWindow3DT& ViewWindow, wxMouseEvent& ME);

    bool IsHiddenByTool(const MapElementT* Elem) const;
    void RenderTool2D(Renderer2DT& Renderer) const;
    void RenderTool3D(Renderer3DT& Renderer) const;

    // ObserverT implementation.
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    void NotifySubjectDies(SubjectT* dyingSubject);

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    enum DragStateT { DragNothing, DragBoxSelection, DragMorphHandles };

    /// Returns the handles of all objects that are currently being morphed.
    ArrayT<MorphHandleT> GetHandles(bool SelectedOnly, bool Vertices=true, bool Edges=true) const;

    /// Finds all morph handles that are in ViewWindow at Point.
    /// Note that this may return a mix of vertex and edge handles (if the tool options bar is set to "edit both"), selected and unselected!
    ///
    /// @param ViewWindow   The window in which we should look at Point for handles.
    /// @param Point        The coordinate of interest in ViewWindow, given in Tool(!) Space.
    /// @returns the array of all morph handles "under" Point in ViewWindow.
    ArrayT<MorphHandleT> GetMorphHandlesAt(ViewWindow2DT& ViewWindow, const wxPoint& Point) const;

    /// Finds the morph handle that is in ViewWindow along the ray through Point.
    /// @param ViewWindow   The window in which we should look along the ray through Point for handles.
    /// @param Point        The coordinate through which the ray of interested is cast into the world, given in Window(!) Space.
    /// @param FoundMH      If a morph handle was found, it is returned via this reference.
    ///    If there is more than one morph handle along the ray through Point, the morph handle that is closest to the viewer is returned.
    /// @returns true if a morph handle was found in ViewWindow along the ray through Point, false if no such handle exists.
    bool GetMorphHandleAt(ViewWindow3DT& ViewWindow, const wxPoint& Point, MorphHandleT& FoundMH) const;

    int  MorphPrims_Find(const MapElementT* Elem) const;        ///< Returns the array index number of the MorphPrimT for the given Elem, -1 if there is none.
    void MorphPrims_SyncTo(const ArrayT<MapElementT*>& Elems);  ///< Sync's our m_MorphPrims to the given Elems, so that they correspond 1:1 to each other.
    void MoveSelectedHandles(const Vector3fT& Delta);           ///< WARNING: This method *DESTROYS* all handle pointers into any of the m_MorphPrims!!
    void NudgeSelectedHandles(const AxesInfoT& AxesInfo, const wxKeyEvent& KE);
    void FinishDragMorphHandles();                              ///< Called from the 2D or 3D views, for morph-modified items, this function replaces the original primitives with the morphed ones in the map.
    void OnEscape(ViewWindowT& ViewWindow);


    // The crucial member variables that define the essential state of this tool are m_MorphPrims and m_DragState.
    //
    // A few general notes about dragging morph (vertex and edge) handles:
    // a) At any time, any number of morph handles may be selected. The user then picks *one* to start dragging them *all*.
    // b) During a drag, the original underlying MorphPrimT(s) can be entirely be re-created and re-formed, due to the nature of the convex-hull algorithm.
    //    Therefore, it's quasi impossible to keep any pointer to anything "inside" the MorphPrimT(s) during the drag,
    //    especially we can NOT keep a pointer to the handle (vertex or edge) that was used to start the drag!!
    //    Fortunately, we don't need any such information, because we just duplicate the current position of
    //    the handle that was chosen to start the drag in the m_DragHandleCurrentPos member.
    ArrayT<MorphPrimT*> m_MorphPrims;               ///< List of primitives currently being morphed by this tool.
    DragStateT          m_DragState;                ///< If we are currently dragging anything (LMB is down), and if so, what.
    Vector3fT           m_DragHandleOrigPos;        ///< If m_DragState==DragMorphHandles, this was the original position of the handle when the drag started.      If m_DragState==DragBoxSelection, this is where dragging the new selection box began.
    Vector3fT           m_DragHandleCurrentPos;     ///< If m_DragState==DragMorphHandles, this is the current position of the handle that was chosen for the drag. If m_DragState==DragBoxSelection, this the current position of the other corner of the selection box.
    bool                m_IsRecursiveSelfNotify;    ///< Whether an observer message has been triggered by the tool itself.

    OptionsBar_EditVerticesToolT* m_OptionsBar;   ///< The options bar for this tool.
};

#endif
