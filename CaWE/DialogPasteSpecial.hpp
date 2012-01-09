/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef _DIALOG_PASTE_SPECIAL_HPP_
#define _DIALOG_PASTE_SPECIAL_HPP_

#include "Math3D/BoundingBox.hpp"


class wxSpinCtrl;


class PasteSpecialDialogT : public wxDialog
{
    public:

    // Constructor.
    PasteSpecialDialogT(const BoundingBox3fT& ObjectsBox);

    int  NrOfCopies;
    bool CenterAtOriginal;
    bool GroupCopies;
    int  TranslateX;
    int  TranslateY;
    int  TranslateZ;
    int  RotateX;
    int  RotateY;
    int  RotateZ;


    private:

    const int ObjectsSizeX;
    const int ObjectsSizeY;
    const int ObjectsSizeZ;

    wxSpinCtrl* SpinCtrlTranslateX;
    wxSpinCtrl* SpinCtrlTranslateY;
    wxSpinCtrl* SpinCtrlTranslateZ;

    // Event handlers.
    void OnButtonGetWidth (wxCommandEvent& Event);
    void OnButtonGetDepth (wxCommandEvent& Event);
    void OnButtonGetHeight(wxCommandEvent& Event);
    void OnButtonOK       (wxCommandEvent& Event);

    // IDs for the controls in whose events we are interested.
    enum
    {
        ID_BUTTON_GET_WIDTH=wxID_HIGHEST+1,
        ID_BUTTON_GET_DEPTH,
        ID_BUTTON_GET_HEIGHT
    };

    DECLARE_EVENT_TABLE()
};

#endif
