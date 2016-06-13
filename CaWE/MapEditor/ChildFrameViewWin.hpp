/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CHILDFRAME_VIEW_WIN_HPP_INCLUDED
#define CAFU_CHILDFRAME_VIEW_WIN_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "ObserverPatternTools.hpp"


class AxesInfoT;
class ChildFrameT;
class MapDocumentT;
class wxWindow;


/// This class represents a (superclass of a) 2D or 3D map view window.
class ViewWindowT : public ObserverT, public ToolsObserverT
{
    public:

    enum ViewTypeT
    {
        VT_2D_XY,           // Top
        VT_2D_XZ,           // Front
        VT_2D_YZ,           // Side
        VT_3D_WIREFRAME,
        VT_3D_FLAT,
        VT_3D_EDIT_MATS,    // Materials in Edit    mode (shows the meta_EditorImage texture).
        VT_3D_FULL_MATS,    // Materials in Preview mode (shows the real material).
        VT_3D_LM_GRID,
        VT_3D_LM_PREVIEW
    };


    // Methods inherited from ObserverT.
 // void NotifySubjectChanged(...);     // Implemented by derived classes.
    void NotifySubjectDies(SubjectT* dyingSubject);

    virtual wxWindow* GetWindow()=0;            ///< This function is not const because we can mutate this(!) object via the returned pointer.
    virtual ViewTypeT GetViewType() const=0;    ///< Returns the view type of this view window.
    virtual AxesInfoT GetAxesInfo() const=0;    ///< This method returns the axes info for this window. In the case of a 3D window, it computes the 2D axes info that this view is closest to.
    wxString          GetCaption() const;       ///< Returns the caption that the AUI pane for this window should have.
    ChildFrameT*      GetChildFrame() const;    ///< Returns the child frame that owns this view (that is, our parent).
    MapDocumentT&     GetMapDoc() const;        ///< The document that is associated with this view window (or more precisely, our child frame).


    protected:

    /// The constructor. It is protected because only child classes should ever be instantiated.
    ViewWindowT(ChildFrameT* ChildFrame);

    /// The destructor. Virtual such that also the child destructors will be called.
    virtual ~ViewWindowT();

    void UpdateChildFrameMRU();

    ChildFrameT* m_ChildFrame;      ///< The child frame that owns us (that is, is our parent).
};

#endif
