/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_CURSOR_MAN_HPP_INCLUDED
#define CAFU_CURSOR_MAN_HPP_INCLUDED

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
