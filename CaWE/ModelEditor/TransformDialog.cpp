/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "TransformDialog.hpp"
#include "ChildFrame.hpp"
#include "ModelDocument.hpp"
#include "Commands/TransformJoint.hpp"
#include "Commands/UpdateAnim.hpp"
#include "Math3D/Angles.hpp"
#include "Math3D/Quaternion.hpp"
#include "Models/Model_cmdl.hpp"

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

    wxStaticText *item1 = new wxStaticText(this, -1, wxT("The transformation is applied to the currently selected animation sequence(s):"), wxDefaultPosition, wxSize(220, 32), 0 );
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


namespace
{
    struct TrafoT
    {
        Vector3fT Trafo[3];
    };
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
            m_Notebook->GetPage(SelPage)->TransferDataFromWindow();

            const ArrayT<unsigned int>& SelAnims=m_ModelDoc->GetSelection(ANIM);

            if (m_ModelDoc->GetModel()->GetJoints().Size()==0)
                break;

            // Transform the bind pose.
            if (SelAnims.Size()==0)
            {
                const ArrayT<CafuModelT::JointT>& Joints=m_ModelDoc->GetModel()->GetJoints();

                switch (SelPage)
                {
                    case 0:
                    {
                        const Vector3fT Values(m_Values[SelPage]);

                        m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, 0, 'p', Joints[0].Pos + Values));
                        break;
                    }

                    case 1:
                    {
                        const Vector3fT Values=Vector3fT(m_Values[SelPage]) * float(cf::math::AnglesT<double>::PI / 180.0);
                        const cf::math::QuaternionfT Qtr=
                            cf::math::QuaternionfT::Euler(Values.y, Values.z, Values.x) *
                            cf::math::QuaternionfT::FromXYZ(Joints[0].Qtr);

                        m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, 0, 'q', Qtr.GetXYZ()));
                        break;
                    }

                    case 2:
                    {
                        Vector3fT Values(m_Values[SelPage]);

                        for (unsigned int i=0; i<3; i++)
                            if (Values[i]==0.0f) Values[i]=1.0f;

                        m_Parent->SubmitCommand(new CommandTransformJointT(m_ModelDoc, 0, 's', scale(Joints[0].Scale, Values)));
                        break;
                    }
                }
            }

            // Transform the selected anims.
            ArrayT<CommandT*> TransformCommands;

            for (unsigned long SelNr=0; SelNr<SelAnims.Size(); SelNr++)
            {
                const CafuModelT::AnimT&             Anim=m_ModelDoc->GetModel()->GetAnims()[SelAnims[SelNr]];
                const CafuModelT::AnimT::AnimJointT& AJ  =Anim.AnimJoints[0];

                // "Decompress" all frames for the given anim joint AJ.
                ArrayT<TrafoT> Decomp;
                Decomp.PushBackEmptyExact(Anim.Frames.Size());

                for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                {
                    Decomp[FrameNr].Trafo[0]=AJ.DefaultPos;
                    Decomp[FrameNr].Trafo[1]=AJ.DefaultQtr;
                    Decomp[FrameNr].Trafo[2]=AJ.DefaultScale;

                    unsigned int FlagCount=0;

                    for (unsigned int i=0; i<9; i++)
                    {
                        if ((AJ.Flags >> i) & 1)
                        {
                            Decomp[FrameNr].Trafo[i/3][i % 3]=Anim.Frames[FrameNr].AnimData[AJ.FirstDataIdx+FlagCount];
                            FlagCount++;
                        }
                    }
                }

                // Transform the decompressed data.
                switch (SelPage)
                {
                    case 0:
                    {
                        const Vector3fT Values(m_Values[SelPage]);

                        for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                            Decomp[FrameNr].Trafo[0] += Values;
                        break;
                    }

                    case 1:
                    {
                        const Vector3fT Values=Vector3fT(m_Values[SelPage]) * float(cf::math::AnglesT<double>::PI / 180.0);
                        const cf::math::QuaternionfT Qtr=cf::math::QuaternionfT::Euler(Values.y, Values.z, Values.x);

                        for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                            Decomp[FrameNr].Trafo[1] = (Qtr * cf::math::QuaternionfT::FromXYZ(Decomp[FrameNr].Trafo[1])).GetXYZ();
                        break;
                    }

                    case 2:
                    {
                        Vector3fT Values(m_Values[SelPage]);

                        for (unsigned int i=0; i<3; i++)
                            if (Values[i]==0.0f) Values[i]=1.0f;

                        for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                            Decomp[FrameNr].Trafo[2] = scale(Decomp[FrameNr].Trafo[2], Values);
                        break;
                    }
                }

                // Create a new CafuModelT::AnimT instance, "recompressing" the transformed data into it.
                CafuModelT::AnimT              NewAnim=Anim;
                CafuModelT::AnimT::AnimJointT& NewAJ  =NewAnim.AnimJoints[0];

                NewAJ.DefaultPos  =Decomp[/*FrameNr*/0].Trafo[0];
                NewAJ.DefaultQtr  =Decomp[/*FrameNr*/0].Trafo[1];
                NewAJ.DefaultScale=Decomp[/*FrameNr*/0].Trafo[2];

                for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                {
                    unsigned int FlagCount=0;

                    for (unsigned int i=0; i<9; i++)
                    {
                        if ((NewAJ.Flags >> i) & 1)
                        {
                            NewAnim.Frames[FrameNr].AnimData[NewAJ.FirstDataIdx+FlagCount]=Decomp[FrameNr].Trafo[i/3][i % 3];
                            FlagCount++;
                        }
                    }

                    NewAnim.RecomputeBB(FrameNr, m_ModelDoc->GetModel()->GetJoints(), m_ModelDoc->GetModel()->GetMeshes());
                }

                TransformCommands.PushBack(new CommandUpdateAnimT(m_ModelDoc, SelAnims[SelNr], NewAnim));
            }

            if (TransformCommands.Size()==1)
            {
                m_Parent->SubmitCommand(TransformCommands[0]);
            }
            else if (TransformCommands.Size()>1)
            {
                m_Parent->SubmitCommand(new CommandMacroT(TransformCommands,
                    wxString::Format("Transform %u anims", TransformCommands.Size())));
            }
            break;
        }
    }
}
