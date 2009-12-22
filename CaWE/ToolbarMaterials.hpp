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

#ifndef _TOOLBAR_MATERIALS_HPP_
#define _TOOLBAR_MATERIALS_HPP_

#include "ObserverPattern.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"


class BitmapControlT;
class MapDocumentT;
class EditorMaterialI;
class EditorMatManT;


/// The Materials toolbar (actually, a dialog).
class MaterialsToolbarT : public wxPanel, public ObserverT
{
    public:

    /// The constructor.
    MaterialsToolbarT(wxWindow* Parent, MapDocumentT* MapDoc);

    // Implementation of the ObserverT interface.
    void NotifySubjectDies(SubjectT* Subject);

    /// Overridden wxDialog::Show() function, because we also want to update the toolbar on Show(true).
    bool Show(bool show=true);

    /// Returns the current list of MRU materials (the first element is the selected material).
    ArrayT<EditorMaterialI*> GetMRUMaterials() const;


    private:

    /// Pointer to the currently active document, or NULL when no document active.
    MapDocumentT*   m_MapDoc;
    EditorMatManT&  m_MatMan;

    /// The controls.
    wxChoice*       ChoiceCurrentMat;
    wxStaticText*   StaticTextCurrentMatSize;
    BitmapControlT* BitmapCurrentMat;

    /// The event handlers.
    void OnSelChangeCurrentMat(wxCommandEvent&  Event);
    void OnButtonBrowse       (wxCommandEvent&  Event);
    void OnButtonReplace      (wxCommandEvent&  Event);
    void OnUpdateUI           (wxUpdateUIEvent& Event);

    /// IDs for the controls in whose events we are interested.
    enum
    {
        ID_CHOICE_CURRENT_MAT=wxID_HIGHEST+1,
        ID_BUTTON_BROWSE_MATS,
        ID_BUTTON_REPLACE_MATS
    };

    DECLARE_EVENT_TABLE()
};

#endif
