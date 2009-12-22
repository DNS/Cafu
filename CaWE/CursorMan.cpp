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

#include "CursorMan.hpp"
#include "wx/cursor.h"

#ifdef __WXGTK__
#include "wx/image.h"
#endif


// Provide and initialize the global CursorMan variable.
// An instance will be allocated and freed by the AppCaWE application object.
CursorManT* CursorMan=NULL;


CursorManT::CursorManT()
{
    // Initialize the list with NULL pointers.
    for (unsigned long CursorNr=0; CursorNr<INVALID; CursorNr++)
        Cursors.PushBack(NULL);

    // Assign the cursors.
    const wxString AppDir="./CaWE";

#ifdef __WXMSW__
    Cursors[CROSS               ]=new wxCursor(AppDir+"/res/CursorCross.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[NEW_ENTITY_TOOL     ]=new wxCursor(AppDir+"/res/CursorToolNewEntity.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[NEW_BRUSH_TOOL      ]=new wxCursor(AppDir+"/res/CursorToolNewBrush.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[NEW_BEZIERPATCH_TOOL]=new wxCursor(AppDir+"/res/CursorToolNewBezierPatch.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[NEW_TERAIN_TOOL     ]=new wxCursor(AppDir+"/res/CursorToolNewTerrain.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[NEW_DECAL_TOOL      ]=new wxCursor(AppDir+"/res/CursorToolNewDecal.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[EDIT_FACEPROPS_TOOL ]=new wxCursor(AppDir+"/res/CursorToolEditFaceProps.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[EYE_DROPPER         ]=new wxCursor(AppDir+"/res/CursorEyedropper.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[HAND_OPEN           ]=new wxCursor(AppDir+"/res/CursorHandOpen.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[HAND_CLOSED         ]=new wxCursor(AppDir+"/res/CursorHandClosed.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[SIZING_PLUS         ]=new wxCursor(AppDir+"/res/CursorSizingPlus.cur", wxBITMAP_TYPE_CUR, 6, 6);
    Cursors[ROTATE              ]=new wxCursor(AppDir+"/res/CursorRotationHandle.cur", wxBITMAP_TYPE_CUR, 15, 15);
#else
    Cursors[CROSS               ]=new wxCursor(wxImage(AppDir+"/res/CursorCross.cur"));
    Cursors[NEW_ENTITY_TOOL     ]=new wxCursor(wxImage(AppDir+"/res/CursorToolNewEntity.cur"));
    Cursors[NEW_BRUSH_TOOL      ]=new wxCursor(wxImage(AppDir+"/res/CursorToolNewBrush.cur"));
    Cursors[NEW_BEZIERPATCH_TOOL]=new wxCursor(wxImage(AppDir+"/res/CursorToolNewBezierPatch.cur"));
    Cursors[NEW_TERAIN_TOOL     ]=new wxCursor(wxImage(AppDir+"/res/CursorToolNewTerrain.cur"));
    Cursors[NEW_DECAL_TOOL      ]=new wxCursor(wxImage(AppDir+"/res/CursorToolNewDecal.cur"));
    Cursors[EDIT_FACEPROPS_TOOL ]=new wxCursor(wxImage(AppDir+"/res/CursorToolEditFaceProps.cur"));
    Cursors[EYE_DROPPER         ]=new wxCursor(wxImage(AppDir+"/res/CursorEyedropper.cur"));
    Cursors[HAND_OPEN           ]=new wxCursor(wxImage(AppDir+"/res/CursorHandOpen.cur"));
    Cursors[HAND_CLOSED         ]=new wxCursor(wxImage(AppDir+"/res/CursorHandClosed.cur"));
    Cursors[SIZING_PLUS         ]=new wxCursor(wxImage(AppDir+"/res/CursorSizingPlus.cur"));
    Cursors[ROTATE              ]=new wxCursor(wxImage(AppDir+"/res/CursorRotationHandle.cur"));
#endif

    // Make sure that all cursors have been assigned.
    for (unsigned long CursorNr=0; CursorNr<Cursors.Size(); CursorNr++)
        wxASSERT(Cursors[CursorNr]!=NULL);
}


CursorManT::~CursorManT()
{
    for (unsigned long CursorNr=0; CursorNr<Cursors.Size(); CursorNr++)
        delete Cursors[CursorNr];
}


const wxCursor& CursorManT::GetCursor(CursorManT::CursorIdentT CursorID) const
{
    return *Cursors[CursorID];
}
