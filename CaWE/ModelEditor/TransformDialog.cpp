/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "TransformDialog.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
//#include "Models/Model_cmdl.hpp"
#include "wx/artprov.h"
#include "wx/imaglist.h"
#include "wx/notebook.h"
#include "wx/valnum.h"


using namespace ModelEditor;


BEGIN_EVENT_TABLE(TransformDialogT, wxPanel)
    EVT_BUTTON(ID_BUTTON_RESET, TransformDialogT::OnButton)
    EVT_BUTTON(ID_BUTTON_APPLY, TransformDialogT::OnButton)
END_EVENT_TABLE()


namespace
{
    const wxString g_Units[] = { "units", "°", "" };
    const wxString g_InitValuesStr[] = { "0.0", "0.0", "1.0" };
    const float    g_InitValuesFlt[] = { 0.0f, 0.0f, 1.0f };

    wxSizer *ModelE_Transform_TrafoInit( wxWindow *parent, unsigned int TrafoNr, float Values[], bool call_fit, bool set_sizer )
    {
        wxFlexGridSizer *item0 = new wxFlexGridSizer( 3, 0, 0 );

        wxStaticText *item1 = new wxStaticText( parent, -1, wxT("x:"), wxDefaultPosition, wxDefaultSize, 0 );
        item0->Add( item1, 0, wxALIGN_CENTER|wxALL, 5 );

        wxTextCtrl *item2 = new wxTextCtrl( parent, -1, g_InitValuesStr[TrafoNr], wxDefaultPosition, wxSize(40,-1), 0, wxFloatingPointValidator<float>(&Values[0], wxNUM_VAL_NO_TRAILING_ZEROES) );
        item0->Add( item2, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

        wxStaticText *item3 = new wxStaticText( parent, -1, g_Units[TrafoNr], wxDefaultPosition, wxDefaultSize, 0 );
        item0->Add( item3, 0, wxALIGN_CENTER|wxRIGHT|wxTOP|wxBOTTOM, 5 );

        wxStaticText *item4 = new wxStaticText( parent, -1, wxT("y:"), wxDefaultPosition, wxDefaultSize, 0 );
        item0->Add( item4, 0, wxALIGN_CENTER|wxALL, 5 );

        wxTextCtrl *item5 = new wxTextCtrl( parent, -1, g_InitValuesStr[TrafoNr], wxDefaultPosition, wxSize(40,-1), 0, wxFloatingPointValidator<float>(&Values[1], wxNUM_VAL_NO_TRAILING_ZEROES) );
        item0->Add( item5, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

        wxStaticText *item6 = new wxStaticText( parent, -1, g_Units[TrafoNr], wxDefaultPosition, wxDefaultSize, 0 );
        item0->Add( item6, 0, wxALIGN_CENTER|wxRIGHT|wxTOP|wxBOTTOM, 5 );

        wxStaticText *item7 = new wxStaticText( parent, -1, wxT("z:"), wxDefaultPosition, wxDefaultSize, 0 );
        item0->Add( item7, 0, wxALIGN_CENTER|wxALL, 5 );

        wxTextCtrl *item8 = new wxTextCtrl( parent, -1, g_InitValuesStr[TrafoNr], wxDefaultPosition, wxSize(40,-1), 0, wxFloatingPointValidator<float>(&Values[2], wxNUM_VAL_NO_TRAILING_ZEROES) );
        item0->Add( item8, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

        wxStaticText *item9 = new wxStaticText( parent, -1, g_Units[TrafoNr], wxDefaultPosition, wxDefaultSize, 0 );
        item0->Add( item9, 0, wxALIGN_CENTER|wxRIGHT|wxTOP|wxBOTTOM, 5 );

        item0->AddGrowableCol( 1 );

        if (set_sizer)
        {
            parent->SetSizer( item0 );
            if (call_fit)
                item0->SetSizeHints( parent );
        }

        return item0;
    }
}


TransformDialogT::TransformDialogT(ChildFrameT* Parent, const wxSize& Size)
    : wxPanel(Parent, wxID_ANY, wxDefaultPosition, Size),
      m_ModelDoc(Parent->GetModelDoc()),
      m_Parent(Parent),
      m_Notebook(NULL)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticText *item1 = new wxStaticText(this, -1, wxT("The transformation is applied to the entire model in the selected animation sequence or bind pose:"), wxDefaultPosition, wxSize(220,48), 0 );
    item0->Add( item1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    m_Notebook=new wxNotebook(this, -1, wxDefaultPosition, wxDefaultSize, 0 );
    wxWindow *item2 = m_Notebook;

    wxImageList* TabImages=new wxImageList(22, 22);

    TabImages->Add(wxArtProvider::GetBitmap("transform-translate",    wxART_TOOLBAR, wxSize(22, 22)));
    TabImages->Add(wxArtProvider::GetBitmap("transform-rotate-right", wxART_TOOLBAR, wxSize(22, 22)));
    TabImages->Add(wxArtProvider::GetBitmap("transform-scale",        wxART_TOOLBAR, wxSize(22, 22)));

    // The wxNotebook takes ownership of the icons list and deletes it later.
    m_Notebook->AssignImageList(TabImages);

    wxPanel *item4 = new wxPanel( m_Notebook, -1 );
    ModelE_Transform_TrafoInit( item4, 0, m_Values[0], true, true);
    m_Notebook->AddPage( item4, wxT("Translate") );
    m_Notebook->SetPageImage(0, 0);

    wxPanel *item5 = new wxPanel( m_Notebook, -1 );
    ModelE_Transform_TrafoInit( item5, 1, m_Values[1], true, true);
    m_Notebook->AddPage( item5, wxT("Rotate") );
    m_Notebook->SetPageImage(1, 1);

    wxPanel *item6 = new wxPanel( m_Notebook, -1 );
    ModelE_Transform_TrafoInit( item6, 2, m_Values[2], true, true);
    m_Notebook->AddPage( item6, wxT("Scale") );
    m_Notebook->SetPageImage(2, 2);

    item0->Add( item2, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxBoxSizer *item7 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item8 = new wxButton(this, ID_BUTTON_RESET, wxT("Reset values"), wxDefaultPosition, wxDefaultSize, 0 );
    item7->Add( item8, 0, wxALIGN_CENTER|wxALL, 5 );

    wxButton *item9 = new wxButton(this, ID_BUTTON_APPLY, wxT("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->SetDefault();
    item7->Add( item9, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item7, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5 );

    SetSizer( item0 );
    // item0->SetSizeHints(this);   // Activating this seems to override the initial "Size" given above.

    m_ModelDoc->RegisterObserver(this);
}


TransformDialogT::~TransformDialogT()
{
    if (m_ModelDoc)
        m_ModelDoc->UnregisterObserver(this);
}


void TransformDialogT::Notify_SelectionChanged(SubjectT* Subject, ModelElementTypeT Type, const ArrayT<unsigned int>& OldSel, const ArrayT<unsigned int>& NewSel)
{
    if (Type!=ANIM) return;

    // RefreshPropGrid();
}


void TransformDialogT::Notify_SubjectDies(SubjectT* dyingSubject)
{
    wxASSERT(dyingSubject==m_ModelDoc);

    m_ModelDoc=NULL;
    // ClearPage(0);
}


void TransformDialogT::OnButton(wxCommandEvent& Event)
{
    const int SelPage=m_Notebook->GetSelection();

    if (SelPage<0 || SelPage>2)
        return;

    switch (Event.GetId())
    {
        case ID_BUTTON_RESET:
        {
            for (unsigned int i=0; i<3; i++)
                m_Values[SelPage][i] = g_InitValuesFlt[SelPage];

            m_Notebook->GetPage(SelPage)->TransferDataToWindow();
            break;
        }

        case ID_BUTTON_APPLY:
        {
            break;
        }
    }
}
