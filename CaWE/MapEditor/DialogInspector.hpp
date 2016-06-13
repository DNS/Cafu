/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_INSPECTOR_HPP_INCLUDED
#define CAFU_DIALOG_INSPECTOR_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "Templates/Array.hpp"
#include "wx/panel.h"


namespace MapEditor { class EntityInspectorDialogT; }
class InspDlgPrimitivePropsT;
class MapDocumentT;
class MapElementT;
class wxNotebook;


class InspectorDialogT : public wxPanel, public ObserverT
{
    public:

    InspectorDialogT(wxWindow* Parent, MapDocumentT* MapDoc);
    ~InspectorDialogT();

    // ObserverT implementation.
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection) override;
    void NotifySubjectDies(SubjectT* dyingSubject) override;


    private:

    MapDocumentT*                      m_MapDoc;
    wxNotebook*                        Notebook;
    MapEditor::EntityInspectorDialogT* m_EntityInspectorDialog;
    InspDlgPrimitivePropsT*            PrimitiveProps;
};

#endif
