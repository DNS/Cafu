/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_RENDERER_2D_HPP_INCLUDED
#define CAFU_RENDERER_2D_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Templates/Array.hpp"

#include "wx/wx.h"


class ViewWindow2DT;


/// This class implements the rendering into a 2D view.
/// Most methods are just simple wrappers around wxDC functionality, but it still makes rendering more convenient.
class Renderer2DT
{
    public:

    static const int LINE_THIN;
    static const int LINE_THICK;

    /// The Constructor.
    Renderer2DT(const ViewWindow2DT& ViewWin2D, wxDC& dc);

    /// Returns the ViewWindow2DT that this renderer renders into.
    const ViewWindow2DT& GetViewWin2D() const { return m_ViewWin2D; }

    void SetFillColor(const wxColour& Color);
    void SetLineColor(const wxColour& Color);
    void SetLineType(wxPenStyle Style, int Width, const wxColour& Color);

    void DrawPoint(const wxPoint& Point, int Radius);
    void DrawPoint(const Vector3fT& Point, int Radius);

    void DrawLine(const wxPoint& A, const wxPoint& B);
    void DrawLine(const Vector3fT& A, const Vector3fT& B);
    void DrawLine(int x1, int y1, int x2, int y2);
    void DrawLineLoop(const ArrayT<Vector3fT>& Points, int Radius);

    void DrawEllipse(const wxPoint& Center, int RadiusX, int RadiusY, bool Fill);
    void Rectangle(const wxRect& Rect, bool Fill);
    void DrawBitmap(int x, int y, const wxBitmap& Bitmap);

 // void SetFont(...);          // TODO!
 // void GetTextExtent(...);    // TODO!, this easily (1:1) implemented with wxDC::GetTextExtent().
    void SetTextColor(const wxColour& FgColor, const wxColour& BkColor);
    void DrawText(const wxString& Text, const wxPoint& Pos);


    /// Renders the basis vectors (the "axes") of the given matrix at the given position with the given length.
    void BasisVectors(const Vector3fT& Pos, const cf::math::Matrix3x3fT& Mat, float Length=24.0f);

    /// Renders an X-shaped handle at the given position with the given radius.
    /// This is typically used for marking the center of map elements.
    void XHandle(const wxPoint& Pos, int Radius=3);

    /// Renders the dimensions of the given bounding-box next to the box.
    /// @param BB    The bounding-box whose dimensions are printed.
    /// @param Pos   A combination of the flags wxTOP, wxBOTTOM, wxLEFT and wxRIGHT that determines where the dimensions are printed.
    void DrawBoxDims(const BoundingBox3fT& BB, int Pos);


    private:

    const ViewWindow2DT& m_ViewWin2D;   ///< The 2D view that this renderer is rendering into.
    wxDC&                m_DC;          ///< The 2D views device context.
    wxPen                m_Pen;         ///< The pen that is currently being used for line drawing.
    wxBrush              m_Brush;       ///< The brush that is currently being used for area filling.
};

#endif
