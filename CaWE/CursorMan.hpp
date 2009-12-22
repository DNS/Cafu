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

#ifndef _CURSOR_MAN_HPP_
#define _CURSOR_MAN_HPP_

#include "Templates/Array.hpp"


class wxCursor;


/// This class manages the cursors of this application.
class CursorManT
{
    public:

    enum CursorIdentT
    {
        CROSS=0,
        NEW_ENTITY_TOOL,
        NEW_BRUSH_TOOL,
        NEW_BEZIERPATCH_TOOL,
        NEW_TERAIN_TOOL,
        NEW_DECAL_TOOL,
        EDIT_FACEPROPS_TOOL,
        EYE_DROPPER,
        HAND_OPEN,
        HAND_CLOSED,
        SIZING_PLUS,
        ROTATE,
        INVALID             // Do not change this, it must always be the last element in the enumeration.
    };


    /// The constructor.
    /// Must only be called after wxWidgets has been initialized.
    CursorManT();

    /// The destructor.
    /// Must be called before wxWidgets is initialized.
    ~CursorManT();

    const wxCursor& GetCursor(CursorIdentT CursorID) const;


    private:

    ArrayT<const wxCursor*> Cursors;
};


/// This gobal variable provides a single cursor manager for application-wide access.
extern CursorManT* CursorMan;

#endif
