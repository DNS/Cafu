/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "wx/wx.h"
#include "EditorMaterialDummy.hpp"


DummyMaterialT::DummyMaterialT(const wxString& Name)
    : m_Name(Name)
{
}


void DummyMaterialT::Draw(wxDC& dc, const wxRect& DestRect, int NameBoxHeight, bool DrawNameBox) const
{
    // Start with drawing the name box.
    wxRect NameBoxRect=DestRect;

    NameBoxRect.y     =DestRect.GetBottom()-NameBoxHeight;
    NameBoxRect.height=NameBoxHeight;

    dc.SetPen(*wxRED_PEN);
    dc.SetBrush(*wxRED_BRUSH);
    dc.DrawRectangle(NameBoxRect);
    dc.DrawText(m_Name, NameBoxRect.x+4, NameBoxRect.y+1);


    static wxBrush DummyBrush(wxColour(64, 64, 64), wxBRUSHSTYLE_CROSSDIAG_HATCH);

    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(DummyBrush);
    dc.DrawRectangle(DestRect.x, DestRect.y, DestRect.width, DestRect.height-NameBoxHeight);
    dc.DrawText("No Material!", DestRect.x+8, DestRect.y+6);
}


MatSys::RenderMaterialT* DummyMaterialT::GetRenderMaterial(bool PreviewMode) const
{
    // No good. Should return something visible.
    return NULL;
}


MaterialT* DummyMaterialT::GetMaterial() const
{
    return NULL;
}
