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


    static wxBrush DummyBrush(wxColour(64, 64, 64), wxCROSSDIAG_HATCH);

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
