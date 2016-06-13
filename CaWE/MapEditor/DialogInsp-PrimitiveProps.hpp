/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DIALOG_INSP_PRIMITIVE_PROPS_HPP_INCLUDED
#define CAFU_DIALOG_INSP_PRIMITIVE_PROPS_HPP_INCLUDED

#include "ObserverPattern.hpp"
#include "wx/panel.h"


class wxPropertyGridManager;
class wxPropertyGridEvent;
class MapDocumentT;
class MapElementT;
class wxStaticText;


class InspDlgPrimitivePropsT : public wxPanel, public ObserverT
{
    public:

    InspDlgPrimitivePropsT(wxWindow* Parent, MapDocumentT* MapDoc);
    ~InspDlgPrimitivePropsT();

    // Implementation of the ObserverT interface.
    void NotifySubjectChanged_Selection(SubjectT* Subject, const ArrayT<MapElementT*>& OldSelection, const ArrayT<MapElementT*>& NewSelection);
    void NotifySubjectChanged_Deleted(SubjectT* Subject, const ArrayT<MapPrimitiveT*>& Primitives);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail);
    void NotifySubjectChanged_Modified(SubjectT* Subject, const ArrayT<MapElementT*>& MapElements, MapElemModDetailE Detail, const ArrayT<BoundingBox3fT>& OldBounds);
    void NotifySubjectDies(SubjectT* dyingSubject);


    private:

    wxSizer* InspectorPrimitivePropsInit(wxWindow* parent, bool call_fit=true, bool set_sizer=true);
    void     UpdateGrid();

    // Event handler methods.
    void OnPropertyGridChanged(wxPropertyGridEvent& Event);

    MapDocumentT*          m_MapDoc;
    wxPropertyGridManager* m_PropMan;
    wxStaticText*          m_SelectionText;   ///< Text that is displayed above the property grid. It shows the number of selected primitives.
    bool                   m_IsRecursiveSelfNotify;
};

#endif
