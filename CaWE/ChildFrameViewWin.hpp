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

#ifndef _CHILDFRAME_VIEW_WIN_HPP_
#define _CHILDFRAME_VIEW_WIN_HPP_

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
        VT_2D_XY,
        VT_2D_YZ,
        VT_2D_XZ,
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
