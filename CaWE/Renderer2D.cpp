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

#include "Renderer2D.hpp"
#include "ChildFrameViewWin2D.hpp"


const int Renderer2DT::LINE_THIN =1;
const int Renderer2DT::LINE_THICK=2;


Renderer2DT::Renderer2DT(const ViewWindow2DT& ViewWin2D, wxDC& dc)
    : m_ViewWin2D(ViewWin2D),
      m_DC(dc),
      m_Pen(*wxRED_PEN),
      m_Brush(*wxBLUE_BRUSH)
{
}


void Renderer2DT::SetFillColor(const wxColour& Color)
{
    m_Brush=wxBrush(Color, wxSOLID);
}


void Renderer2DT::SetLineColor(const wxColour& Color)
{
    m_Pen.SetColour(Color);
    m_DC.SetPen(m_Pen);
}


void Renderer2DT::SetLineType(wxPenStyle Style, int Width, const wxColour& Color)
{
    m_Pen.SetStyle(Style);
    m_Pen.SetWidth(Width);

    SetLineColor(Color);
}


void Renderer2DT::DrawPoint(const wxPoint& Point, int Radius)
{
    m_Brush.SetStyle(wxSOLID);
    m_DC.SetBrush(m_Brush);
    m_DC.DrawRectangle(Point.x-Radius, Point.y-Radius, Radius*2+1, Radius*2+1);
}


void Renderer2DT::DrawPoint(const Vector3fT& Point, int Radius)
{
    DrawPoint(m_ViewWin2D.WorldToTool(Point), Radius);
}


void Renderer2DT::DrawLine(const wxPoint& A, const wxPoint& B)
{
    m_DC.DrawLine(A.x, A.y, B.x, B.y);
}


void Renderer2DT::DrawLine(const Vector3fT& A, const Vector3fT& B)
{
    DrawLine(m_ViewWin2D.WorldToTool(A), m_ViewWin2D.WorldToTool(B));
}


void Renderer2DT::DrawLine(int x1, int y1, int x2, int y2)
{
    m_DC.DrawLine(x1, y1, x2, y2);
}


void Renderer2DT::DrawLineLoop(const ArrayT<Vector3fT>& Points, int Radius)
{
    for (unsigned long i=0; i<Points.Size(); i++)
    {
        wxPoint screenPoint1=m_ViewWin2D.WorldToTool(Points[i]);
        wxPoint screenPoint2=m_ViewWin2D.WorldToTool(Points[(i+1) % Points.Size()]);

        m_DC.DrawLine(screenPoint1.x, screenPoint1.y, screenPoint2.x, screenPoint2.y);
    }

    if (Radius>0)
        for (unsigned long i=0; i<Points.Size(); i++)
            DrawPoint(m_ViewWin2D.WorldToTool(Points[i]), Radius);
}


void Renderer2DT::DrawEllipse(const wxPoint& Center, int RadiusX, int RadiusY, bool Fill)
{
    m_Brush.SetStyle(Fill ? wxSOLID : wxTRANSPARENT);
    m_DC.SetBrush(m_Brush);

    wxRect Rect(Center.x-RadiusX, Center.y-RadiusY, 2*RadiusX, 2*RadiusY);
    m_DC.DrawEllipse(Rect);
}


void Renderer2DT::Rectangle(const wxRect& Rect, bool Fill)
{
    m_Brush.SetStyle(Fill ? wxSOLID : wxTRANSPARENT);
    m_DC.SetBrush(m_Brush);

    m_DC.DrawRectangle(Rect.x, Rect.y, Rect.width, Rect.height);
}


void Renderer2DT::DrawBitmap(int x, int y, const wxBitmap& Bitmap)
{
    m_DC.DrawBitmap(Bitmap, x, y, false);
}


void Renderer2DT::SetTextColor(const wxColour& FgColor, const wxColour& BkColor)
{
    m_DC.SetTextForeground(FgColor);
    m_DC.SetTextBackground(BkColor);
}


void Renderer2DT::DrawText(const wxString& Text, const wxPoint& Pos)
{
    m_DC.SetBackgroundMode(wxTRANSPARENT);  // TODO: Make this a param to this method!
    m_DC.DrawText(Text, Pos.x, Pos.y);
}


void Renderer2DT::BasisVectors(const Vector3fT& Pos, const cf::math::Matrix3x3fT& Mat, float Length)
{
    SetLineType(wxPENSTYLE_SOLID, LINE_THIN, wxColour(255, 0, 0));
    DrawLine(Pos, Pos + Vector3fT(Mat[0][0], Mat[1][0], Mat[2][0])*Length);

    SetLineType(wxPENSTYLE_SOLID, LINE_THIN, wxColour(0, 255, 0));
    DrawLine(Pos, Pos + Vector3fT(Mat[0][1], Mat[1][1], Mat[2][1])*Length);

    SetLineType(wxPENSTYLE_SOLID, LINE_THIN, wxColour(0, 0, 255));
    DrawLine(Pos, Pos + Vector3fT(Mat[0][2], Mat[1][2], Mat[2][2])*Length);
}


void Renderer2DT::XHandle(const wxPoint& Pos, int Radius)
{
    DrawLine(Pos.x-Radius, Pos.y-Radius, Pos.x+Radius+1, Pos.y+Radius+1);
    DrawLine(Pos.x+Radius, Pos.y-Radius, Pos.x-Radius-1, Pos.y+Radius+1);
}


/// Checks if a rectangluar text area with a fixed height and width lies within a window viewport defined by two points.
/// The text areas point of origin is adjusted, so the whole text lies inside the viewport.
static wxPoint Clamp(const wxRect& TextRect, const wxPoint& WinMin, const wxPoint& WinMax)
{
    wxPoint newPos(TextRect.x, TextRect.y);

    // Check if the text areas bounding lies outside of right and bottom viewport border and adjust its position.
    if (newPos.x>WinMax.x-TextRect.width ) newPos.x=WinMax.x-TextRect.width;
    if (newPos.y>WinMax.y-TextRect.height) newPos.y=WinMax.y-TextRect.height;

    // Check if the text areas origin lies outside of left and upper viewport border and adjust its position.
    // Note that the upper and left border are adjusted after the bottom and right border, so a text area that won't fit
    // in the window at all, will always be justified with the left and upper border.
    if (newPos.x<WinMin.x) newPos.x=WinMin.x;
    if (newPos.y<WinMin.y) newPos.y=WinMin.y;

    return newPos;
}


void Renderer2DT::DrawBoxDims(const BoundingBox3fT& BB, int Flags)
{
    const int HorzAxis=GetViewWin2D().GetAxesInfo().HorzAxis;
    const int VertAxis=GetViewWin2D().GetAxesInfo().VertAxis;

    const wxRect RectBB(GetViewWin2D().WorldToTool(BB.Min),
                        GetViewWin2D().WorldToTool(BB.Max));

    wxRect TextRect;
    TextRect.width =75;
    TextRect.height=15;
    SetTextColor(wxColour(255, 255, 255), wxColour(0, 0, 0));

    // Get the tool coordinates of the upper left (1, 1) and the lower right (n, n) window coordinates.
    // These are needed to adjust the text position, so they won't be rendered outside the view window.
    const wxSize  ClientSize  =GetViewWin2D().GetClientSize();
    const wxPoint WinMin      =GetViewWin2D().WindowToTool(wxPoint(1, 1));
    const wxPoint WinMax      =GetViewWin2D().WindowToTool(wxPoint(ClientSize.GetWidth(), ClientSize.GetHeight()));
    const wxString AxesNames[]={ "x:", "y:", "z:" };


    // Print horizontal extension value.
    const wxString HorzText=AxesNames[HorzAxis]+wxString::Format(" %.2f", BB.Max[HorzAxis] - BB.Min[HorzAxis]);

    // Set the horizontal text position centered to the objects/selection horizontal axis.
    TextRect.x=RectBB.x + RectBB.width/2 - TextRect.width/2;
    TextRect.y=(Flags & wxTOP) ? (RectBB.y-TextRect.height-5) : (RectBB.y+RectBB.height+5);
    DrawText(HorzText, Clamp(TextRect, WinMin, WinMax));


    // Print vertical extension value.
    const wxString VertText=AxesNames[VertAxis]+wxString::Format(" %.2f", BB.Max[VertAxis] - BB.Min[VertAxis]);

    // Set the vertical text position centered to the objects/selection vertical axis.
    TextRect.x=(Flags & wxLEFT) ? (RectBB.x-TextRect.width-5) : (RectBB.x+RectBB.width+5);
    TextRect.y=RectBB.y + RectBB.height/2 - TextRect.height/2;
    DrawText(VertText, Clamp(TextRect, WinMin, WinMax));
}
