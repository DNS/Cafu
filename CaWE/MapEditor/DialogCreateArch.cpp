/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "DialogCreateArch.hpp"
#include "MapBrush.hpp"

#include "../AxesInfo.hpp"
#include "../Options.hpp"

#include "Math3D/Misc.hpp"

#include "wx/spinctrl.h"
#include "wx/valgen.h"

#include <algorithm>


class ArchDialogT::PreviewWinT : public wxWindow
{
    public:

    PreviewWinT(ArchDialogT* Parent, wxWindowID id, const wxPoint& Pos, const wxSize& Size);


    private:

    ArchDialogT* m_Parent;

    void OnPaint(wxPaintEvent& event);

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(ArchDialogT::PreviewWinT, wxWindow)
    EVT_PAINT(ArchDialogT::PreviewWinT::OnPaint)
END_EVENT_TABLE()


ArchDialogT::PreviewWinT::PreviewWinT(ArchDialogT* Parent, wxWindowID id, const wxPoint& Pos, const wxSize& Size)
    : wxWindow(Parent, id, Pos, Size),
      m_Parent(Parent)
{
    SetOwnBackgroundColour(*wxBLACK);
}


static void DrawLine(wxPaintDC& dc, const Vector3fT& A, const Vector3fT& B)
{
    dc.DrawLine(A.x, A.y, B.x, B.y);
}


void ArchDialogT::PreviewWinT::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    dc.SetPen(*wxLIGHT_GREY_PEN);

    const wxSize    ClSize    =GetClientSize();
    const Vector3fT ClCenter  =Vector3fT(ClSize.GetWidth()/2, ClSize.GetHeight()/2, 0.0f);
    const Vector3fT ArchSize  =m_Parent->m_BB.Max - m_Parent->m_BB.Min;
    const float     Scale     =std::min(ClSize.GetWidth()/ArchSize.x, ClSize.GetHeight()/ArchSize.y);
    const Vector3fT ArchRadius=ArchSize/2.0f * Scale;
    const float     AlphaStep =float(m_Parent->m_Arc)/m_Parent->m_NrOfSegments;

    for (int SegmentNr=0; SegmentNr<m_Parent->m_NrOfSegments; SegmentNr++)
    {
        const float Alpha1=m_Parent->m_StartAngle+AlphaStep*SegmentNr;
        const float Alpha2=Alpha1+AlphaStep;

        const Vector3fT Ray1=Vector3fT(0, 1, 0).GetRotZ(-Alpha1);   // Rotate clockwise.
        const Vector3fT Ray2=Vector3fT(0, 1, 0).GetRotZ(-Alpha2);

        const float Radius1=1.0f/sqrt(Ray1.x*Ray1.x/(ArchRadius.x*ArchRadius.x) + Ray1.y*Ray1.y/(ArchRadius.y*ArchRadius.y));
        const float Radius2=1.0f/sqrt(Ray2.x*Ray2.x/(ArchRadius.x*ArchRadius.x) + Ray2.y*Ray2.y/(ArchRadius.y*ArchRadius.y));

        const Vector3fT Offset=Vector3fT(ClCenter.x, ClCenter.y, m_Parent->m_AddHeight*SegmentNr);

        ArrayT<Vector3fT> Vertices;

        Vertices.PushBack(Offset + Ray1*Radius1);
        Vertices.PushBack(Offset + Ray1*std::max(Radius1-m_Parent->m_WallWidth*Scale, 0.0f));
        Vertices.PushBack(Offset + Ray2*Radius2);
        Vertices.PushBack(Offset + Ray2*std::max(Radius2-m_Parent->m_WallWidth*Scale, 0.0f));

        if (SegmentNr==0) DrawLine(dc, Vertices[0], Vertices[1]);
        DrawLine(dc, Vertices[0], Vertices[2]);
        DrawLine(dc, Vertices[1], Vertices[3]);
        DrawLine(dc, Vertices[2], Vertices[3]);
    }
}


BEGIN_EVENT_TABLE(ArchDialogT, wxDialog)
    EVT_SPINCTRL(-1, ArchDialogT::OnSpinEvent)
END_EVENT_TABLE()


ArchDialogT::ArchDialogT(const BoundingBox3fT& BB, const AxesInfoT& AxesInfo, wxWindow* Parent)
    : wxDialog(Parent, -1, "Arch Properties", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_BB(BB),
      m_WallWidth(32),
      m_NrOfSegments(8),
      m_Arc(180),
      m_StartAngle(0),
      m_AddHeight(0)
{
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *item1 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticBox *item3 = new wxStaticBox( this, -1, wxT("Preview") );
    wxStaticBoxSizer *item2 = new wxStaticBoxSizer( item3, wxVERTICAL );

    m_PreviewWin = new PreviewWinT(this, -1, wxDefaultPosition, wxSize(150, 150));
    item2->Add( m_PreviewWin, 1, wxALL|wxEXPAND, 2 );

    item1->Add( item2, 1, wxALL|wxEXPAND, 5 );

    wxStaticBox *item8 = new wxStaticBox( this, -1, wxT("Properties") );
    wxStaticBoxSizer *item7 = new wxStaticBoxSizer( item8, wxVERTICAL );

    wxGridSizer *item9 = new wxGridSizer( 2, 0, 0 );

    wxStaticText *item10 = new wxStaticText( this, -1, wxT("Wall Width:"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->Add( item10, 0, wxALIGN_RIGHT|wxALL, 5 );

    const Vector3fT ArchSize=m_BB.Max-m_BB.Min;
    wxSpinCtrl* SpinC_WallWidth = new wxSpinCtrl( this, -1, wxT("50"), wxDefaultPosition, wxSize(75,-1), wxSP_ARROW_KEYS, 2, std::min(ArchSize[AxesInfo.HorzAxis], ArchSize[AxesInfo.VertAxis]), m_WallWidth);
    SpinC_WallWidth->SetValidator(wxGenericValidator(&m_WallWidth));
    item9->Add( SpinC_WallWidth, 0, wxALIGN_RIGHT|wxALL, 5 );

    wxStaticText *item12 = new wxStaticText( this, -1, wxT("Number of Segments:"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->Add( item12, 0, wxALIGN_RIGHT|wxALL, 5 );

    wxSpinCtrl* SpinC_NrOfSegments = new wxSpinCtrl( this, -1, wxT("8"), wxDefaultPosition, wxSize(75,-1), wxSP_ARROW_KEYS, 3, 100, m_NrOfSegments);
    SpinC_NrOfSegments->SetValidator(wxGenericValidator(&m_NrOfSegments));
    item9->Add( SpinC_NrOfSegments, 0, wxALIGN_RIGHT|wxALL, 5 );

    wxStaticText *item14 = new wxStaticText( this, -1, wxT("Arc:"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->Add( item14, 0, wxALIGN_RIGHT|wxALL, 5 );

    wxSpinCtrl* SpinC_Arc = new wxSpinCtrl( this, -1, wxT("180"), wxDefaultPosition, wxSize(75,-1), wxSP_ARROW_KEYS, 0, 360, m_Arc);
    SpinC_Arc->SetValidator(wxGenericValidator(&m_Arc));
    item9->Add( SpinC_Arc, 0, wxALIGN_RIGHT|wxALL, 5 );

    wxStaticText *item16 = new wxStaticText( this, -1, wxT("Start Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->Add( item16, 0, wxALIGN_RIGHT|wxALL, 5 );

    wxSpinCtrl* SpinC_StartAngle = new wxSpinCtrl( this, -1, wxT("0"), wxDefaultPosition, wxSize(75,-1), wxSP_ARROW_KEYS, 0, 360, m_StartAngle);
    SpinC_StartAngle->SetValidator(wxGenericValidator(&m_StartAngle));
    item9->Add( SpinC_StartAngle, 0, wxALIGN_RIGHT|wxALL, 5 );

    wxStaticText *item18 = new wxStaticText( this, -1, wxT("Add Height:"), wxDefaultPosition, wxDefaultSize, 0 );
    item9->Add( item18, 0, wxALIGN_RIGHT|wxALL, 5 );

    wxSpinCtrl* SpinC_AddHeight = new wxSpinCtrl( this, -1, wxT("0"), wxDefaultPosition, wxSize(75,-1), wxSP_ARROW_KEYS, 0, 2048, m_AddHeight);
    SpinC_AddHeight->SetValidator(wxGenericValidator(&m_AddHeight));
    item9->Add( SpinC_AddHeight, 0, wxALIGN_RIGHT|wxALL, 5 );

    item7->Add( item9, 0, wxALIGN_CENTER|wxALL, 5 );
    item1->Add( item7, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );
    item0->Add( item1, 1, wxALL|wxEXPAND, 5 );

    wxBoxSizer *item20 = new wxBoxSizer( wxHORIZONTAL );

    wxButton *item21 = new wxButton( this, wxID_CANCEL, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    item20->Add( item21, 0, wxALIGN_CENTER|wxALL|wxEXPAND, 5 );

    wxButton *item22 = new wxButton( this, wxID_OK, wxT("Create"), wxDefaultPosition, wxDefaultSize, 0 );
    item20->Add( item22, 0, wxALIGN_CENTER|wxALL, 5 );

    item0->Add( item20, 0, wxALIGN_CENTER|wxALL, 5 );

    this->SetSizer( item0 );
    item0->SetSizeHints( this );
}


ArrayT<MapPrimitiveT*> ArchDialogT::GetArch(EditorMaterialI* Material) const
{
    const Vector3fT ArchCenter=m_BB.GetCenter();
    const Vector3fT ArchRadius=(m_BB.Max-m_BB.Min)/2.0f;
    const float     AlphaStep =float(m_Arc)/m_NrOfSegments;

    ArrayT<MapPrimitiveT*> Segments;

    for (int SegmentNr=0; SegmentNr<m_NrOfSegments; SegmentNr++)
    {
        const float Alpha1=m_StartAngle+AlphaStep*SegmentNr;
        const float Alpha2=Alpha1+AlphaStep;

        const Vector3fT Ray1=Vector3fT(0, 1, 0).GetRotZ(-Alpha1);   // Rotate clockwise.
        const Vector3fT Ray2=Vector3fT(0, 1, 0).GetRotZ(-Alpha2);

        const float Radius1=1.0f/sqrt(Ray1.x*Ray1.x/(ArchRadius.x*ArchRadius.x) + Ray1.y*Ray1.y/(ArchRadius.y*ArchRadius.y));
        const float Radius2=1.0f/sqrt(Ray2.x*Ray2.x/(ArchRadius.x*ArchRadius.x) + Ray2.y*Ray2.y/(ArchRadius.y*ArchRadius.y));

        const Vector3fT Offset=Vector3fT(ArchCenter.x, ArchCenter.y, m_AddHeight*SegmentNr);

        ArrayT<Vector3fT> Vertices;

        Vertices.PushBack(Offset + Ray1*Radius1);
        Vertices.PushBack(Offset + Ray1*std::max(Radius1-m_WallWidth, 0.0f));
        Vertices.PushBack(Offset + Ray2*Radius2);
        Vertices.PushBack(Offset + Ray2*std::max(Radius2-m_WallWidth, 0.0f));

        for (unsigned long VertexNr=0; VertexNr<4; VertexNr++)
        {
            Vertices.PushBack(Vertices[VertexNr]);

            Vertices[VertexNr  ].z+=m_BB.Min.z;
            Vertices[VertexNr+4].z+=m_BB.Max.z;
        }

        // Build the convex hull over the Vertices.
        MapBrushT* NewBrush=new MapBrushT(Vertices, Material, Options.general.NewUVsFaceAligned);

        if (!NewBrush->IsValid())
        {
            wxASSERT(false);
            delete NewBrush;
            continue;
        }

        Segments.PushBack(NewBrush);
    }

    if (Segments.Size()==0)
    {
        Segments.PushBack(MapBrushT::CreateBlock(m_BB, Material));
    }

    return Segments;
}


void ArchDialogT::OnSpinEvent(wxSpinEvent& event)
{
    if (Validate() && TransferDataFromWindow())
        m_PreviewWin->Refresh();
}
