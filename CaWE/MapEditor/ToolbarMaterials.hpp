/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TOOLBAR_MATERIALS_HPP_INCLUDED
#define CAFU_TOOLBAR_MATERIALS_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "Templates/Array.hpp"
#include "wx/wx.h"


class MapDocumentT;
class EditorMaterialI;
class EditorMatManT;


/// The Materials toolbar (actually, a dialog).
class MaterialsToolbarT : public wxPanel, public ObserverT
{
    public:

    /// The constructor.
    MaterialsToolbarT(wxWindow* Parent, MapDocumentT* MapDoc);

    /// The destructor.
    ~MaterialsToolbarT();

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
    wxStaticBitmap* m_BitmapCurrentMat;

    /// The event handlers.
    void OnSelChangeCurrentMat(wxCommandEvent&  Event);
    void OnButtonBrowse       (wxCommandEvent&  Event);
    void OnButtonApply        (wxCommandEvent&  Event);
    void OnButtonReplace      (wxCommandEvent&  Event);
    void OnUpdateUI           (wxUpdateUIEvent& Event);

    /// IDs for the controls in whose events we are interested.
    enum
    {
        ID_CHOICE_CURRENT_MAT=wxID_HIGHEST+1,
        ID_BUTTON_BROWSE_MATS,
        ID_BUTTON_APPLY,
        ID_BUTTON_REPLACE_MATS
    };

    DECLARE_EVENT_TABLE()
};

#endif
