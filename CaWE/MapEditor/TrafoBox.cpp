/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "TrafoBox.hpp"
#include "ChildFrame.hpp"
#include "ChildFrameViewWin2D.hpp"
#include "MapDocument.hpp"
#include "MapElement.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"

#include "../CursorMan.hpp"
#include "../Options.hpp"

#include "Commands/Transform.hpp"

#include "Math3D/Angles.hpp"
#include "Math3D/Misc.hpp"

#include <algorithm>


const TrafoBoxT::TrafoHandleT TrafoBoxT::HandleTable[3][3][3]=
{
    // The handles for trafo mode TM_SCALE.
    {
        { TH_TOP_LEFT,    TH_TOP,    TH_TOP_RIGHT },
        { TH_LEFT,        TH_NONE,   TH_RIGHT },
        { TH_BOTTOM_LEFT, TH_BOTTOM, TH_BOTTOM_RIGHT }
    },

    // The handles for trafo mode TM_ROTATE.
    {
        { TH_TOP_LEFT,    TH_NONE, TH_TOP_RIGHT },
        { TH_NONE,        TH_NONE, TH_NONE },
        { TH_BOTTOM_LEFT, TH_NONE, TH_BOTTOM_RIGHT }
    },

    // The handles for trafo mode TM_SHEAR.
    {
        { TH_NONE, TH_TOP,    TH_NONE },
        { TH_LEFT, TH_NONE,   TH_RIGHT },
        { TH_NONE, TH_BOTTOM, TH_NONE }
    }
};

static const int HandlePadding=6;
static const int HandleRadius =4;


TrafoBoxT::TrafoBoxT()
    : m_BB(),
      m_ExtraRefPos(),
      m_TrafoMode(TM_SCALE),
      m_DragState(TH_NONE),
      m_DragAxes(0, false, 1, false),
      m_LDownPosWorld(),
      m_RefPos(),
      m_Translate(),
      m_Scale(),
      m_RotAngle(0.0f),
      m_Shear(0.0f)
{
}


void TrafoBoxT::SetBB(const BoundingBox3fT& BB, const ArrayT<Vector3fT>& ExtraRefPos)
{
    // Can only reset the m_BB when no drag is currently in progress.
    wxASSERT(m_DragState == TH_NONE);
    if (m_DragState != TH_NONE) return;

    m_BB = BB;

    // Don't use `m_ExtraRefPos = ExtraRefPos;` in order to avoid memory reallocation.
    m_ExtraRefPos.Overwrite();

    for (unsigned int i = 0; i < ExtraRefPos.Size(); i++)
        m_ExtraRefPos.PushBack(ExtraRefPos[i]);

    if (m_ExtraRefPos.Size() == 0 && m_BB.IsInited())
        m_ExtraRefPos.PushBack(m_BB.GetCenter());
}


void TrafoBoxT::SetTrafoMode(TrafoModeT TM)
{
    // Can only change the transformation mode when no drag is currently in progress.
    wxASSERT(m_DragState == TH_NONE);
    if (m_DragState != TH_NONE) return;

    m_TrafoMode = TM;
}


void TrafoBoxT::SetNextTrafoMode()
{
    // Can only change the transformation mode when no drag is currently in progress.
    wxASSERT(m_DragState==TH_NONE);
    if (m_DragState!=TH_NONE) return;

    switch (m_TrafoMode)
    {
        case TM_SCALE:  m_TrafoMode=TM_ROTATE; break;
        case TM_ROTATE: m_TrafoMode=TM_SHEAR;  break;
        case TM_SHEAR:  m_TrafoMode=TM_SCALE;  break;
    }
}


TrafoBoxT::TrafoHandleT TrafoBoxT::CheckForHandle(const ViewWindow2DT& ViewWindow, const wxPoint& PointTS) const
{
    // When m_BB is not initialized, PointTS cannot be over a handle.
    if (!m_BB.IsInited()) return TH_NONE;

    const wxRect Rect=wxRect(ViewWindow.WorldToTool(m_BB.Min), ViewWindow.WorldToTool(m_BB.Max));

    // Check if PointTS is "inside" of m_BB.
    if (Rect.Contains(PointTS)) return TH_BODY;

    // Check which handle PointTS is over.
    int          BestDist  =HandleRadius*HandleRadius+1;
    TrafoHandleT BestHandle=TH_NONE;

    for (int y=0; y<3; y++)
        for (int x=0; x<3; x++)
        {
            if (HandleTable[m_TrafoMode][y][x]==TH_NONE) continue;

            const wxPoint HandlePos(Rect.x + Rect.width /2*x + (x-1)*HandlePadding,
                                    Rect.y + Rect.height/2*y + (y-1)*HandlePadding);
            const wxPoint Delta=HandlePos-PointTS;
            const int     Dist =Delta.x*Delta.x + Delta.y*Delta.y;

            if (Dist<BestDist)
            {
                BestDist  =Dist;
                BestHandle=HandleTable[m_TrafoMode][y][x];
            }
        }

    return BestHandle;
}


wxCursor TrafoBoxT::SuggestCursor(TrafoHandleT TrafoHandle) const
{
    if (TrafoHandle==TH_BODY)
    {
        return wxCursor(wxCURSOR_SIZING);
    }

    if (m_TrafoMode==TM_SCALE)
    {
        switch (TrafoHandle)
        {
            case TH_TOP_LEFT:
            case TH_BOTTOM_RIGHT:
                return wxCursor(wxCURSOR_SIZENWSE);

            case TH_TOP_RIGHT:
            case TH_BOTTOM_LEFT:
                return wxCursor(wxCURSOR_SIZENESW);

            case TH_RIGHT:
            case TH_LEFT:
                return wxCursor(wxCURSOR_SIZEWE);

            case TH_TOP:
            case TH_BOTTOM:
                return wxCursor(wxCURSOR_SIZENS);

            default:
                break;
        }
    }

    if (m_TrafoMode==TM_ROTATE)
    {
        return CursorMan->GetCursor(CursorManT::ROTATE);
    }

    if (m_TrafoMode==TM_SHEAR)
    {
        switch (TrafoHandle)
        {
            case TH_RIGHT:
            case TH_LEFT:
                return wxCursor(wxCURSOR_SIZENS);

            case TH_TOP:
            case TH_BOTTOM:
                return wxCursor(wxCURSOR_SIZEWE);

            default:
                break;
        }
    }

    return *wxSTANDARD_CURSOR;
}


static float GetDistSqr(float x1, float y1, float x2, float y2)
{
    const float a=x2-x1;
    const float b=y2-y1;

    return a*a + b*b;
}


static float GetAngle(float x1, float y1, float x2, float y2)
{
    const float a=x2-x1;
    const float b=y2-y1;

    return (a==0.0f && b==0.0f) ? 0.0f : cf::math::AnglesfT::RadToDeg(atan2(b, a));
}


bool TrafoBoxT::BeginTrafo(const ViewWindow2DT& ViewWindow, const wxPoint& PointTS /*Tool Space*/)
{
    const int HorzAxis =ViewWindow.GetAxesInfo().HorzAxis;
    const int VertAxis =ViewWindow.GetAxesInfo().VertAxis;
    const int ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;

    // When m_BB is not initialized, we cannot begin a transformation.
    // Note that this check is redundant - CheckForHandle() returns TH_NONE when m_BB is not initialized anyway.
    if (!m_BB.IsInited()) return false;

    // Cannot begin a new transformation when another one is currently in progress.
    wxASSERT(m_DragState == TH_NONE);
    if (m_DragState != TH_NONE) return false;

    m_DragState     = CheckForHandle(ViewWindow, PointTS);
    m_DragAxes      = ViewWindow.GetAxesInfo();
    m_LDownPosWorld = ViewWindow.ToolToWorld(PointTS, 0.0f);    // m_LDownPosWorld[ThirdAxis] is set to 0.
    m_RefPos        = m_BB.GetCenter();                         // Re-assigned below if necessary for the specific trafo.
    m_Translate     = Vector3fT(0.0f, 0.0f, 0.0f);
    m_Scale         = Vector3fT(1.0f, 1.0f, 1.0f);
    m_RotAngle      = 0.0f;
    m_Shear         = 0.0f;

    // Cannot begin a new transformation when the mouse is not over anything.
    // This is intentionally checked late, so that the above values are updated
    // even if `m_DragState == TH_NONE`.
    if (m_DragState == TH_NONE)
    {
        return false;
    }

    // We've started dragging the body.
    if (m_DragState==TH_BODY)
    {
        // Set m_RefPos to the corner of m_BB that is nearest to m_LDownPosWorld (all in world space).
        float SmallestDist;

        float Dist=GetDistSqr(m_BB.Min[HorzAxis], m_BB.Min[VertAxis], m_LDownPosWorld[HorzAxis], m_LDownPosWorld[VertAxis]);
        if (true)
        {
            SmallestDist=Dist;
            m_RefPos[HorzAxis]=m_BB.Min[HorzAxis];
            m_RefPos[VertAxis]=m_BB.Min[VertAxis];
        }

        Dist=GetDistSqr(m_BB.Max[HorzAxis], m_BB.Min[VertAxis], m_LDownPosWorld[HorzAxis], m_LDownPosWorld[VertAxis]);
        if (Dist<SmallestDist)
        {
            SmallestDist=Dist;
            m_RefPos[HorzAxis]=m_BB.Max[HorzAxis];
            m_RefPos[VertAxis]=m_BB.Min[VertAxis];
        }

        Dist=GetDistSqr(m_BB.Max[HorzAxis], m_BB.Max[VertAxis], m_LDownPosWorld[HorzAxis], m_LDownPosWorld[VertAxis]);
        if (Dist<SmallestDist)
        {
            SmallestDist=Dist;
            m_RefPos[HorzAxis]=m_BB.Max[HorzAxis];
            m_RefPos[VertAxis]=m_BB.Max[VertAxis];
        }

        Dist=GetDistSqr(m_BB.Min[HorzAxis], m_BB.Max[VertAxis], m_LDownPosWorld[HorzAxis], m_LDownPosWorld[VertAxis]);
        if (Dist<SmallestDist)
        {
            SmallestDist=Dist;
            m_RefPos[HorzAxis]=m_BB.Min[HorzAxis];
            m_RefPos[VertAxis]=m_BB.Max[VertAxis];
        }

        for (unsigned int i = 0; i < m_ExtraRefPos.Size(); i++)
        {
            const Vector3fT& Pos = m_ExtraRefPos[i];
            Dist = GetDistSqr(Pos[HorzAxis], Pos[VertAxis], m_LDownPosWorld[HorzAxis], m_LDownPosWorld[VertAxis]);

            if (Dist < SmallestDist)
            {
                SmallestDist = Dist;
                m_RefPos[HorzAxis] = Pos[HorzAxis];
                m_RefPos[VertAxis] = Pos[VertAxis];
            }
        }

        m_RefPos[ThirdAxis]=0;
        return true;
    }

    // We've started dragging one of the handles (not the body).
    if (m_TrafoMode==TM_SCALE)
    {
        // The m_RefPos is the corner opposite the grabbed handle.
        const TrafoHandleT HandleWS=GetWorldSpaceHandle(m_DragState, m_DragAxes);

        // Make sure that the components of m_RefPos in unscaled (1.0) directions is 0,
        // although any value would work.
        m_RefPos=Vector3fT(0, 0, 0);

        if      (HandleWS & TH_LEFT  ) m_RefPos[HorzAxis]=m_BB.Max[HorzAxis];
        else if (HandleWS & TH_RIGHT ) m_RefPos[HorzAxis]=m_BB.Min[HorzAxis];

        if      (HandleWS & TH_TOP   ) m_RefPos[VertAxis]=m_BB.Max[VertAxis];
        else if (HandleWS & TH_BOTTOM) m_RefPos[VertAxis]=m_BB.Min[VertAxis];
    }

    return true;
}


bool TrafoBoxT::UpdateTrafo(const ViewWindow2DT& ViewWindow, const wxPoint& PointTS, bool ToggleGrid)
{
    const int HorzAxis =ViewWindow.GetAxesInfo().HorzAxis;
    const int VertAxis =ViewWindow.GetAxesInfo().VertAxis;
    const int ThirdAxis=ViewWindow.GetAxesInfo().ThirdAxis;

    const Vector3fT MousePosWorld=ViewWindow.ToolToWorld(PointTS, 0.0f);    // MouseWorldPos[ThirdAxis] is set to 0.

    if (m_DragState==TH_NONE)
    {
        return false;
    }

    if (m_DragState==TH_BODY)
    {
        const Vector3fT OldTranslate=m_Translate;

        // Consider the vector from m_LDownPosWorld to MousePosWorld, add this difference to our m_RefPos, and snap that.
        // The difference to m_RefPos is our desired translation delta.
        m_Translate=ViewWindow.GetMapDoc().SnapToGrid(m_RefPos + (MousePosWorld-m_LDownPosWorld), ToggleGrid, ThirdAxis) - m_RefPos;
        wxASSERT(m_Translate[ThirdAxis]==0.0f);

        return m_Translate!=OldTranslate;
    }

    if (m_TrafoMode==TM_SCALE)
    {
        const Vector3fT    OldScale=m_Scale;
        const Vector3fT    BBSize  =m_BB.Max-m_BB.Min;
        const float        MinSize =(ViewWindow.GetMapDoc().IsSnapEnabled()!=ToggleGrid) ? ViewWindow.GetMapDoc().GetGridSpacing() : 1.0f;
        const TrafoHandleT HandleWS=GetWorldSpaceHandle(m_DragState, m_DragAxes);

        wxASSERT(m_Scale[ThirdAxis]==1.0f);

        if (BBSize[VertAxis]>0)
        {
            if (HandleWS & TH_TOP)
            {
                const float NewPos =ViewWindow.GetMapDoc().SnapToGrid(m_BB.Min[VertAxis] + (MousePosWorld[VertAxis]-m_LDownPosWorld[VertAxis]), ToggleGrid);
                const float NewPos2=std::min(NewPos, m_BB.Max[VertAxis]-MinSize);

                m_Scale[VertAxis]=(m_BB.Max[VertAxis]-NewPos2)/BBSize[VertAxis];
            }
            else if (HandleWS & TH_BOTTOM)
            {
                const float NewPos =ViewWindow.GetMapDoc().SnapToGrid(m_BB.Max[VertAxis] + (MousePosWorld[VertAxis]-m_LDownPosWorld[VertAxis]), ToggleGrid);
                const float NewPos2=std::max(NewPos, m_BB.Min[VertAxis]+MinSize);

                m_Scale[VertAxis]=(NewPos2-m_BB.Min[VertAxis])/BBSize[VertAxis];
            }
        }

        if (BBSize[HorzAxis]>0)
        {
            if (HandleWS & TH_LEFT)
            {
                const float NewPos =ViewWindow.GetMapDoc().SnapToGrid(m_BB.Min[HorzAxis] + (MousePosWorld[HorzAxis]-m_LDownPosWorld[HorzAxis]), ToggleGrid);
                const float NewPos2=std::min(NewPos, m_BB.Max[HorzAxis]-MinSize);

                m_Scale[HorzAxis]=(m_BB.Max[HorzAxis]-NewPos2)/BBSize[HorzAxis];
            }
            else if (HandleWS & TH_RIGHT)
            {
                const float NewPos =ViewWindow.GetMapDoc().SnapToGrid(m_BB.Max[HorzAxis] + (MousePosWorld[HorzAxis]-m_LDownPosWorld[HorzAxis]), ToggleGrid);
                const float NewPos2=std::max(NewPos, m_BB.Min[HorzAxis]+MinSize);

                m_Scale[HorzAxis]=(NewPos2-m_BB.Min[HorzAxis])/BBSize[HorzAxis];
            }
        }

        return m_Scale!=OldScale;
    }

    if (m_TrafoMode==TM_ROTATE)
    {
        const float OldRotAngle=m_RotAngle;
        const float RoundTo=(ViewWindow.GetMapDoc().IsSnapEnabled()!=ToggleGrid) ? 15.0f : 0.5f;    // "!=" implements "xor" for bools.

        m_RotAngle=GetAngle(m_RefPos[HorzAxis], m_RefPos[VertAxis], MousePosWorld[HorzAxis], MousePosWorld[VertAxis])
                  -GetAngle(m_RefPos[HorzAxis], m_RefPos[VertAxis], m_LDownPosWorld[HorzAxis], m_LDownPosWorld[VertAxis]);

        // Round to the next 15.0 or 0.5 degrees.
        m_RotAngle=cf::math::round(m_RotAngle/RoundTo)*RoundTo;

        // Make sure that 0 <= m_RotAngle < 360.
        // Could use fmod(), but it doesn't work as desired with negative values of m_RotAngle.
        while (m_RotAngle>=360.0f) m_RotAngle-=360.0f;
        while (m_RotAngle<   0.0f) m_RotAngle+=360.0f;

        return m_RotAngle!=OldRotAngle;
    }

    if (m_TrafoMode==TM_SHEAR)
    {
        const float OldShear=m_Shear;
        const int   Axis    =(m_DragState==TH_TOP || m_DragState==TH_BOTTOM) ? HorzAxis : VertAxis;

        m_Shear=ViewWindow.GetMapDoc().SnapToGrid(MousePosWorld[Axis]-m_LDownPosWorld[Axis], ToggleGrid);

        return m_Shear!=OldShear;
    }

    return false;
}


CommandTransformT* TrafoBoxT::GetTrafoCommand(MapDocumentT& MapDoc, bool LockTexCoords) const
{
    const int HorzAxis  = m_DragAxes.HorzAxis;
    const int VertAxis  = m_DragAxes.VertAxis;
    const int ThirdAxis = m_DragAxes.ThirdAxis;

    if (m_DragState == TH_NONE) return NULL;

    if (m_DragState == TH_BODY)
    {
        wxASSERT(m_Translate[ThirdAxis] == 0.0f);   // Don't change anything along the third axis.

        if (m_Translate[HorzAxis] != 0 || m_Translate[VertAxis] != 0)
            return new CommandTransformT(MapDoc, MapDoc.GetSelection(), CommandTransformT::MODE_TRANSLATE, Vector3fT(), m_Translate, LockTexCoords);
    }
    else
    {
        switch (m_TrafoMode)
        {
            case TM_SCALE:
            {
                wxASSERT((m_DragState & (TH_LEFT | TH_RIGHT )) || m_Scale[HorzAxis] == 1.0f);
                wxASSERT((m_DragState & (TH_TOP  | TH_BOTTOM)) || m_Scale[VertAxis] == 1.0f);
                wxASSERT(m_Scale[ThirdAxis] == 1.0f);

                if (m_Scale[HorzAxis] != 1.0f || m_Scale[VertAxis] != 1.0f)
                    return new CommandTransformT(MapDoc, MapDoc.GetSelection(), CommandTransformT::MODE_SCALE, m_RefPos, m_Scale, LockTexCoords);
                break;
            }

            case TM_ROTATE:
            {
                Vector3fT Angles;
                Angles[ThirdAxis]=m_RotAngle;

                if (Angles[ThirdAxis] != 0)
                    return new CommandTransformT(MapDoc, MapDoc.GetSelection(), CommandTransformT::MODE_ROTATE, m_RefPos, Angles, LockTexCoords);
                break;
            }

            case TM_SHEAR:
            {
                MatrixT ShearMatrix;

                if (GetShearMatrix(ShearMatrix))
                    return new CommandTransformT(MapDoc, MapDoc.GetSelection(), ShearMatrix, LockTexCoords);
                break;
            }
        }
    }

    return NULL;
}


void TrafoBoxT::ApplyTrafo(MapElementT* Elem, bool LockTexCoords) const
{
    const int HorzAxis  = m_DragAxes.HorzAxis;
    const int VertAxis  = m_DragAxes.VertAxis;
    const int ThirdAxis = m_DragAxes.ThirdAxis;

    if (m_DragState == TH_NONE) return;

    if (m_DragState == TH_BODY)
    {
        wxASSERT(m_Translate[ThirdAxis] == 0.0f);   // Don't change anything along the third axis.

        if (m_Translate[HorzAxis] != 0 || m_Translate[VertAxis] != 0)
            Elem->TrafoMove(m_Translate, LockTexCoords);
    }
    else
    {
        switch (m_TrafoMode)
        {
            case TM_SCALE:
            {
                wxASSERT((m_DragState & (TH_LEFT | TH_RIGHT )) || m_Scale[HorzAxis] == 1.0f);
                wxASSERT((m_DragState & (TH_TOP  | TH_BOTTOM)) || m_Scale[VertAxis] == 1.0f);
                wxASSERT(m_Scale[ThirdAxis] == 1.0f);

                if (m_Scale[HorzAxis] != 1.0f || m_Scale[VertAxis] != 1.0f)
                    Elem->TrafoScale(m_RefPos, m_Scale, LockTexCoords);
                break;
            }

            case TM_ROTATE:
            {
                Vector3fT Angles;
                Angles[ThirdAxis]=m_RotAngle;

                if (Angles[ThirdAxis] != 0)
                    Elem->TrafoRotate(m_RefPos, Angles, LockTexCoords);
                break;
            }

            case TM_SHEAR:
            {
                MatrixT ShearMatrix;

                if (GetShearMatrix(ShearMatrix))
                    Elem->Transform(ShearMatrix, LockTexCoords);
                break;
            }
        }
    }
}


void TrafoBoxT::FinishTrafo()
{
    m_DragState=TH_NONE;
}


void TrafoBoxT::RenderRefPosHint(Renderer2DT& Renderer) const
{
    const ViewWindow2DT& ViewWindow = Renderer.GetViewWin2D();

    if (ViewWindow.GetAxesInfo().ThirdAxis != m_DragAxes.ThirdAxis)
        return;

    if (m_DragState == TH_NONE)
    {
        // This assumes that even in the TH_NONE (idle) state, our member variables are
        // properly updated. The Selection tool achieves this by calling BeginTrafo() and
        // FinishTrafo() appropriately.
        // Note that there is little use in keeping something like `m_LDownPosTS`. While this
        // avoids the round-off error from conversion, it is also specific to the ViewWindow's
        // tool space it was recorded at. Despite compatible axes, the tool space of other
        // ViewWindows is still different, rendering a pre-stored `m_LDownPosTS` useless.
        const wxPoint      LDownPosTS = ViewWindow.WorldToTool(m_LDownPosWorld);
        const TrafoHandleT PossibleDragState = CheckForHandle(ViewWindow, LDownPosTS);

        if (PossibleDragState != TH_BODY)
        {
            // There is no need to indicate/preview the possible RefPos for TH_BODY dragging
            // if such a drag state is at the current mouse position out of the question anyway.
            return;
        }
    }
    else
    {
        // If the drag state is TH_BODY, there is no doubt that we should render the
        // RefPos indicator, because that is natural whenever we drag by TH_BODY.
        wxASSERT(m_DragState == TH_BODY);
    }

    const wxPoint RefPos = ViewWindow.WorldToTool(m_RefPos + m_Translate);

    Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColor(128, 128, 0));   // "dirty yellow"

    Renderer.DrawLine(RefPos + wxPoint(-14, 0), RefPos + wxPoint(-4, 0));
    Renderer.DrawLine(RefPos + wxPoint(  5, 0), RefPos + wxPoint(15, 0));
    Renderer.DrawLine(RefPos + wxPoint(0, -14), RefPos + wxPoint(0, -4));
    Renderer.DrawLine(RefPos + wxPoint(0,   5), RefPos + wxPoint(0, 15));
}


void TrafoBoxT::Render(Renderer2DT& Renderer, const wxColour& OutlineColor, const wxColour& HandleColor) const
{
    // When m_BB is not initialized, draw nothing.
    if (!m_BB.IsInited()) return;

    const ViewWindow2DT& ViewWindow=Renderer.GetViewWin2D();

    // The basic outline rectangle for the current, untransformed dimensions (m_BB) of this TrafoBoxT.
    const wxRect Rect(ViewWindow.WorldToTool(m_BB.Min), ViewWindow.WorldToTool(m_BB.Max));


    if (m_DragState==TH_NONE)
    {
        // Draw the basic outline rectangle.
        Renderer.SetLineType(wxPENSTYLE_DOT, Renderer2DT::LINE_THIN, OutlineColor);
        Renderer.Rectangle(Rect, false);

        Renderer.DrawBoxDims(m_BB, wxTOP | wxLEFT);

        // Draw the RefPos indicator (if TH_BODY dragging would be possible).
        RenderRefPosHint(Renderer);

        // Draw the handles corresponding to the current m_TrafoMode.
        Renderer.SetFillColor(HandleColor);
        Renderer.SetLineType(wxPENSTYLE_SOLID, Renderer2DT::LINE_THIN, wxColour(0, 0, 0));

        for (int y=0; y<3; y++)
            for (int x=0; x<3; x++)
            {
                if (HandleTable[m_TrafoMode][y][x]==TH_NONE) continue;

                const wxPoint HandlePos(Rect.x + Rect.width /2*x + (x-1)*HandlePadding,
                                        Rect.y + Rect.height/2*y + (y-1)*HandlePadding);

                if (m_TrafoMode==TM_ROTATE)
                {
                    // Draw a round handle at HandlePos.
                    Renderer.DrawEllipse(HandlePos, HandleRadius, HandleRadius, true);
                }
                else
                {
                    // Draw a square handle at HandlePos.
                    const wxRect HandleRect(HandlePos.x-HandleRadius, HandlePos.y-HandleRadius, 2*HandleRadius, 2*HandleRadius);

                    Renderer.Rectangle(HandleRect, true);
                }
            }

        return;
    }


    Renderer.SetLineType(wxPENSTYLE_DOT, Renderer2DT::LINE_THIN, Options.colors.ToolDrag);

    if (m_DragState==TH_BODY)
    {
        const BoundingBox3fT TranslatedBB(m_BB.Min+m_Translate,
                                          m_BB.Max+m_Translate);

        const wxRect TranslatedRect(ViewWindow.WorldToTool(TranslatedBB.Min),
                                    ViewWindow.WorldToTool(TranslatedBB.Max));

        Renderer.Rectangle(TranslatedRect, false);
        Renderer.DrawBoxDims(TranslatedBB, wxTOP | wxLEFT);

        RenderRefPosHint(Renderer);
        return;
    }

    if (m_TrafoMode==TM_SCALE)
    {
        const BoundingBox3fT ScaledBB((m_BB.Min-m_RefPos).GetScaled(m_Scale)+m_RefPos,
                                      (m_BB.Max-m_RefPos).GetScaled(m_Scale)+m_RefPos);

        const wxRect ScaledRect(ViewWindow.WorldToTool(ScaledBB.Min),
                                ViewWindow.WorldToTool(ScaledBB.Max));

        Renderer.Rectangle(ScaledRect, false);
        Renderer.DrawBoxDims(ScaledBB, wxTOP | wxLEFT);
        return;
    }

    if (m_TrafoMode==TM_ROTATE)
    {
        // Draw the preview of our rotated outline
        // (but only in 2D views that are "axes-compatible" to the one in which the rotation is performed).
        if (ViewWindow.GetAxesInfo().ThirdAxis!=m_DragAxes.ThirdAxis) return;

        // Note that m_RotAngle is in "world space" - must "convert" it to "tool space" first.
        const float RotateAngle=(m_DragAxes.MirrorHorz!=m_DragAxes.MirrorVert) ? -m_RotAngle : m_RotAngle;  // "!=" implements "xor" for bools.

        wxRect r=Rect;
        wxPoint Point=ViewWindow.WorldToTool(m_RefPos);
        Point.x=-Point.x;
        Point.y=-Point.y;
        r.Offset(Point);

        Vector3fT RotResult=Vector3fT(r.x, r.y, 0.0f).GetRotZ(RotateAngle);
        int x1=int(RotResult.x) - Point.x;
        int y1=int(RotResult.y) - Point.y;

        RotResult=Vector3fT(r.x+r.width, r.y, 0.0f).GetRotZ(RotateAngle);
        int x2=int(RotResult.x) - Point.x;
        int y2=int(RotResult.y) - Point.y;

        RotResult=Vector3fT(r.x+r.width, r.y+r.height, 0.0f).GetRotZ(RotateAngle);
        int x3=int(RotResult.x) - Point.x;
        int y3=int(RotResult.y) - Point.y;

        RotResult=Vector3fT(r.x, r.y+r.height, 0.0f).GetRotZ(RotateAngle);
        int x4=int(RotResult.x) - Point.x;
        int y4=int(RotResult.y) - Point.y;

        Renderer.DrawLine(x1, y1, x2, y2);
        Renderer.DrawLine(x2, y2, x3, y3);
        Renderer.DrawLine(x3, y3, x4, y4);
        Renderer.DrawLine(x4, y4, x1, y1);

        // Draw the BB dimensions of the unrotated rectangle only.
        Renderer.DrawBoxDims(m_BB, wxTOP | wxLEFT);
        return;
    }

    if (m_TrafoMode==TM_SHEAR)
    {
        // Draw the preview of our sheared outline
        // (but only in 2D views that are "axes-compatible" to the one in which the shear is performed).
        if (ViewWindow.GetAxesInfo().ThirdAxis!=m_DragAxes.ThirdAxis) return;

        int Shear=int(m_Shear*ViewWindow.GetZoom());        // Convert the m_Shear from world space...
        wxRect r=Rect;

        if (m_DragState==TH_TOP)
        {
            if (m_DragAxes.MirrorHorz) Shear=-Shear;        // ... to tool space.

            Renderer.DrawLine(r.x + Shear, r.y, r.x+r.width + Shear, r.y);
            Renderer.DrawLine(r.x+r.width + Shear, r.y, r.x+r.width, r.y+r.height);
            Renderer.DrawLine(r.x+r.width, r.y+r.height, r.x, r.y+r.height);
            Renderer.DrawLine(r.x, r.y+r.height, r.x + Shear, r.y);
        }
        else if (m_DragState==TH_BOTTOM)
        {
            if (m_DragAxes.MirrorHorz) Shear=-Shear;

            Renderer.DrawLine(r.x, r.y, r.x+r.width, r.y);
            Renderer.DrawLine(r.x+r.width, r.y, r.x+r.width + Shear, r.y+r.height);
            Renderer.DrawLine(r.x+r.width + Shear, r.y+r.height, r.x + Shear, r.y+r.height);
            Renderer.DrawLine(r.x + Shear, r.y+r.height, r.x, r.y);
        }
        else if (m_DragState==TH_LEFT)
        {
            if (m_DragAxes.MirrorVert) Shear=-Shear;

            Renderer.DrawLine(r.x+r.width, r.y, r.x+r.width, r.y+r.height);
            Renderer.DrawLine(r.x+r.width, r.y+r.height, r.x, r.y+r.height + Shear);
            Renderer.DrawLine(r.x, r.y+r.height + Shear, r.x, r.y + Shear);
            Renderer.DrawLine(r.x, r.y + Shear, r.x+r.width, r.y);
        }
        else if (m_DragState==TH_RIGHT)
        {
            if (m_DragAxes.MirrorVert) Shear=-Shear;

            Renderer.DrawLine(r.x, r.y+r.height, r.x, r.y);
            Renderer.DrawLine(r.x, r.y, r.x+r.width, r.y + Shear);
            Renderer.DrawLine(r.x+r.width, r.y + Shear, r.x+r.width, r.y+r.height + Shear);
            Renderer.DrawLine(r.x+r.width, r.y+r.height + Shear, r.x, r.y+r.height);
        }


        // Draw the BB dimensions of the sheared rectangle.
        // Note however that in general, the BB of the sheared m_BB can be different from
        // the BB of the sheared actual map elements in the selection...
        BoundingBox3fT ShearedBB(m_BB);

        switch (m_DragState)
        {
            case TH_TOP:
            case TH_BOTTOM:
                if (m_Shear<0) ShearedBB.Min[m_DragAxes.HorzAxis]+=m_Shear;
                          else ShearedBB.Max[m_DragAxes.HorzAxis]+=m_Shear;
                break;

            case TH_LEFT:
            case TH_RIGHT:
                if (m_Shear<0) ShearedBB.Min[m_DragAxes.VertAxis]+=m_Shear;
                          else ShearedBB.Max[m_DragAxes.VertAxis]+=m_Shear;
                break;

            default:
                break;
        }

        Renderer.DrawBoxDims(ShearedBB, wxTOP | wxLEFT);
        return;
    }
}


// TODO: Support handles also in the 3D view...
void TrafoBoxT::Render(Renderer3DT& Renderer, const wxColour& OutlineColor, const wxColour& HandleColor) const
{
    // When m_BB is not initialized, draw nothing.
    if (!m_BB.IsInited()) return;

    if (m_DragState==TH_NONE)
    {
        Renderer.RenderBox(m_BB, OutlineColor, false /* Solid? */);
        return;
    }

    if (m_DragState==TH_BODY)
    {
        const Vector3fT MinTrans=m_BB.Min+m_Translate;
        const Vector3fT MaxTrans=m_BB.Max+m_Translate;

        Renderer.RenderBox(BoundingBox3fT(MinTrans, MaxTrans), OutlineColor, false /* Solid? */);
        return;
    }

    if (m_TrafoMode==TM_SCALE)
    {
        const Vector3fT MinScaled=(m_BB.Min-m_RefPos).GetScaled(m_Scale)+m_RefPos;
        const Vector3fT MaxScaled=(m_BB.Max-m_RefPos).GetScaled(m_Scale)+m_RefPos;

        Renderer.RenderBox(BoundingBox3fT(MinScaled, MaxScaled), OutlineColor, false /* Solid? */);
        return;
    }

    if (m_TrafoMode==TM_ROTATE)
    {
        Vector3fT BBVertices[8];
        m_BB.GetCornerVertices(BBVertices);

        for (unsigned long VertexNr=0; VertexNr<8; VertexNr++)
        {
            BBVertices[VertexNr]-=m_RefPos;

                 if (m_DragAxes.ThirdAxis==0) BBVertices[VertexNr]=BBVertices[VertexNr].GetRotX( m_RotAngle);
            else if (m_DragAxes.ThirdAxis==1) BBVertices[VertexNr]=BBVertices[VertexNr].GetRotY(-m_RotAngle);
            else if (m_DragAxes.ThirdAxis==2) BBVertices[VertexNr]=BBVertices[VertexNr].GetRotZ( m_RotAngle);

            BBVertices[VertexNr]+=m_RefPos;
        }

        Renderer.RenderBox(BBVertices, OutlineColor, false /* Solid? */);
        return;
    }

    if (m_TrafoMode==TM_SHEAR)
    {
        Vector3fT BBVertices[8];
        MatrixT   ShearMatrix;

        m_BB.GetCornerVertices(BBVertices);

        if (GetShearMatrix(ShearMatrix))
        {
            for (unsigned long VertexNr=0; VertexNr<8; VertexNr++)
                BBVertices[VertexNr]=ShearMatrix.Mul1(BBVertices[VertexNr]);
        }

        Renderer.RenderBox(BBVertices, OutlineColor, false /* Solid? */);
        return;
    }
}


bool TrafoBoxT::UpdateStatusBar(ChildFrameT* ChildFrame) const
{
    // When m_BB is not initialized, do nothing.
    if (!m_BB.IsInited()) return false;

    if (m_DragState==TH_NONE)
    {
        const Vector3fT BBSize=m_BB.Max-m_BB.Min;

        ChildFrame->SetStatusText(wxString::Format(" Size: %.1f, %.1f, %.1f", BBSize.x, BBSize.y, BBSize.z), ChildFrameT::SBP_SELECTION_DIMS);
        return true;
    }

    if (m_DragState==TH_BODY)
    {
        ChildFrame->SetStatusText(wxString::Format(" Move: %.1f, %.1f, %.1f", m_Translate.x, m_Translate.y, m_Translate.z), ChildFrameT::SBP_SELECTION_DIMS);
        return true;
    }

    if (m_TrafoMode==TM_SCALE)
    {
        const Vector3fT BBSize=(m_BB.Max-m_BB.Min).GetScaled(m_Scale);

        ChildFrame->SetStatusText(wxString::Format(" Resize: %.1f, %.1f, %.1f", BBSize.x, BBSize.y, BBSize.z), ChildFrameT::SBP_SELECTION_DIMS);
        return true;
    }

    if (m_TrafoMode==TM_ROTATE)
    {
        // Note that m_RotAngle is in "world space".
        // That is, the user will see the same angle for the same rotation even when the rotation is performed
        // in "mirrored" (invertex axes) views (e.g. one view from "front", one view from "back").
        // Let's hope that there is no confusion when this is sometimes different from the "window space" angle.
        ChildFrame->SetStatusText(wxString::Format(" Rotate: %.1f°", m_RotAngle), ChildFrameT::SBP_SELECTION_DIMS);
        return true;
    }

    if (m_TrafoMode==TM_SHEAR)
    {
        ChildFrame->SetStatusText(wxString::Format(" Shear: %.1f ", m_Shear), ChildFrameT::SBP_SELECTION_DIMS);
        return true;
    }

    return false;
}


/*static*/ TrafoBoxT::TrafoHandleT TrafoBoxT::GetWorldSpaceHandle(TrafoBoxT::TrafoHandleT WindowSpaceHandle, const AxesInfoT& Axes)
{
    unsigned long WorldSpaceHandle=WindowSpaceHandle;

    if (Axes.MirrorHorz && (WorldSpaceHandle & (TH_LEFT | TH_RIGHT))>0)
    {
        // Flip the TH_LEFT and TH_RIGHT bits.
        WorldSpaceHandle^=(TH_LEFT | TH_RIGHT);
    }

    if (Axes.MirrorVert && (WorldSpaceHandle & (TH_TOP | TH_BOTTOM))>0)
    {
        // Flip the TH_TOP and TH_BOTTOM bits.
        WorldSpaceHandle^=(TH_TOP | TH_BOTTOM);
    }

    return TrafoHandleT(WorldSpaceHandle);
}


bool TrafoBoxT::GetShearMatrix(MatrixT& ShearMatrix) const
{
    const int   HorzAxis   =m_DragAxes.HorzAxis;
    const int   VertAxis   =m_DragAxes.VertAxis;
    const float BoxSizeHorz=m_BB.Max[HorzAxis]-m_BB.Min[HorzAxis];
    const float BoxSizeVert=m_BB.Max[VertAxis]-m_BB.Min[VertAxis];

    TrafoHandleT HandleWS=GetWorldSpaceHandle(m_DragState, m_DragAxes);
    bool         IsValid=false;

    if (HandleWS==TH_TOP && BoxSizeVert>0)
    {
        const float Shear=-m_Shear/BoxSizeVert;

        ShearMatrix[HorzAxis][VertAxis]=Shear;
        ShearMatrix[HorzAxis][    3   ]=-m_BB.Max[VertAxis]*Shear;

        IsValid=(Shear!=0);
    }
    else if (HandleWS==TH_BOTTOM && BoxSizeVert>0)
    {
        const float Shear= m_Shear/BoxSizeVert;

        ShearMatrix[HorzAxis][VertAxis]=Shear;
        ShearMatrix[HorzAxis][    3   ]=-m_BB.Min[VertAxis]*Shear;

        IsValid=(Shear!=0);
    }
    else if (HandleWS==TH_LEFT && BoxSizeHorz>0)
    {
        const float Shear=-m_Shear/BoxSizeHorz;

        ShearMatrix[VertAxis][HorzAxis]=Shear;
        ShearMatrix[VertAxis][    3   ]=-m_BB.Max[HorzAxis]*Shear;

        IsValid=(Shear!=0);
    }
    else if (HandleWS==TH_RIGHT && BoxSizeHorz>0)
    {
        const float Shear= m_Shear/BoxSizeHorz;

        ShearMatrix[VertAxis][HorzAxis]=Shear;
        ShearMatrix[VertAxis][    3   ]=-m_BB.Min[HorzAxis]*Shear;

        IsValid=(Shear!=0);
    }

    return IsValid;
}
