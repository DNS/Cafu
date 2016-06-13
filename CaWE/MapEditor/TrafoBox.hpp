/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TRANSFORMATION_BOX_HPP_INCLUDED
#define CAFU_TRANSFORMATION_BOX_HPP_INCLUDED

#include "../AxesInfo.hpp"
#include "Math3D/BoundingBox.hpp"
#include "wx/gdicmn.h"


class ChildFrameT;
class CommandTransformT;
class MapDocumentT;
class MapElementT;
class Renderer2DT;
class Renderer3DT;
class ViewWindow2DT;
template<class T> class Matrix4x4T;
typedef Matrix4x4T<float> Matrix4x4fT;


/// This class implements a spatial box that can be used to define a transformation (translation, rotation, scale or shear).
/// A command for the command history can be created (see method GetTransformCommand()) that actually applies the
/// transformation to a set of map elements.
class TrafoBoxT
{
    public:

    enum TrafoModeT
    {
        TM_SCALE =0,    // The numbering is important, because these constants are also used as array indices.
        TM_ROTATE=1,
        TM_SHEAR =2
    };

    enum TrafoHandleT
    {
        TH_NONE        =0,
        TH_BODY        =1,
     // TH_CENTER      =2,  // For future use, e.g. drag the origin of rotation.

        TH_TOP         =0x10,
        TH_BOTTOM      =0x20,
        TH_LEFT        =0x40,
        TH_RIGHT       =0x80,

        TH_TOP_LEFT    =TH_TOP    | TH_LEFT,
        TH_TOP_RIGHT   =TH_TOP    | TH_RIGHT,
        TH_BOTTOM_LEFT =TH_BOTTOM | TH_LEFT,
        TH_BOTTOM_RIGHT=TH_BOTTOM | TH_RIGHT
    };


    /// The constructor.
    TrafoBoxT();

    /// Returns the spatial dimensions of this trafo box.
    const BoundingBox3fT& GetBB() const { return m_BB; }

    /// Sets new spatial dimensions for this trafo box. Can only be called when no drag is currently in progress (GetDragState() returns TH_NONE).
    void SetBB(const BoundingBox3fT& BB, const ArrayT<Vector3fT>& ExtraRefPos);

    /// Sets the specified transformation mode. Can only be called when no drag is currently in progress (GetDragState() returns TH_NONE).
    void SetTrafoMode(TrafoModeT TM);

    /// Cycles the transformation modes. Can only be called when no drag is currently in progress (GetDragState() returns TH_NONE).
    void SetNextTrafoMode();

    /// Returns which of our handles (if any) is currently being dragged by the user.
    TrafoHandleT GetDragState() const { return m_DragState; }


    /// Returns the handle under the given point PointTS in ViewWindow, TH_NONE if there is none
    /// (only handles matching the current trafo mode are considered).
    ///
    /// @param ViewWindow   The 2D window in which we should search for hits.
    /// @param PointTS      The coordinate in tool space at which we are to look for a handle.
    ///
    /// @returns the handle that was hit in ViewWindow at Point.
    TrafoHandleT CheckForHandle(const ViewWindow2DT& ViewWindow, const wxPoint& PointTS) const;

    /// For the current trafo mode and the given trafo handle, this method returns the related matching
    /// cursor that should be activated in the related view window.
    wxCursor SuggestCursor(TrafoHandleT TrafoHandle) const;


    bool BeginTrafo(const ViewWindow2DT& ViewWindow, const wxPoint& PointTS);
    bool UpdateTrafo(const ViewWindow2DT& ViewWindow, const wxPoint& PointTS, bool ToggleGrid);

    /// This method creates a transform command, according to the current state of the box.
    /// IMPORTANT NOTE: This method must be called after a call to BeginTrafo() and *before* the matching call to EndTrafo()!
    /// @returns the generated transform command, or NULL if no command could be generated.
    CommandTransformT* GetTrafoCommand(MapDocumentT& MapDoc, bool LockTexCoords) const;

    /// Like GetTrafoCommand(), but applies the transformation to the given element immediately.
    void ApplyTrafo(MapElementT* Elem, bool LockTexCoords) const;

    void FinishTrafo();


    void Render(Renderer2DT& Renderer, const wxColour& OutlineColor, const wxColour& HandleColor) const;
    void Render(Renderer3DT& Renderer, const wxColour& OutlineColor, const wxColour& HandleColor) const;
    bool UpdateStatusBar(ChildFrameT* ChildFrame) const;


    private:

    /// A lookup table for valid handles for each transformation mode.
    static const TrafoHandleT HandleTable[3][3][3];

    /// Converts transformation handles from window to world space.
    /// Our transformation handles are normally in "window space" (the origin is in the upper left corner,
    /// the x-axis points right and the y-axis points down), and our understanding of the TrafoHandleT names
    /// (e.g. TH_TOP_LEFT, TH_BOTTOM_RIGHT etc.) is accordingly.
    /// Note that we say "top" is where the vertical coordinates get smaller, "left" is where the horizontal coordinates get smaller!
    /// However, "world space" axes can be opposite the "window space" axes, i.e. inverted, and thus a handle that is "top left"
    /// in window space could in fact be (for example) "bottom right" in world space. Therefore, this function computes from a given
    /// handle in window space the proper handle in world space.
    static TrafoHandleT GetWorldSpaceHandle(TrafoHandleT WindowSpaceHandle, const AxesInfoT& Axes);

    void RenderRefPosHint(Renderer2DT& Renderer) const;

    /// Computes the matrix for the shear transformation that is currently in progress.
    /// @param ShearMatrix   The computed matrix is returned here.
    /// @returns whether the shear matrix could be computed.
    bool GetShearMatrix(Matrix4x4fT& ShearMatrix) const;

    // The overall state of the box is defined by these members:
    BoundingBox3fT    m_BB;             ///< The spatial dimensions of the transformation box. m_BB.IsInited()==false is possible, e.g. when there is no selection.
    ArrayT<Vector3fT> m_ExtraRefPos;    ///< Extra reference points that we may use (besides the m_BB corners) for grid snapping in TH_BODY dragging mode.
    TrafoModeT        m_TrafoMode;      ///< The mode of transformation that the box is currently in: scale, rotate or shear. Translation is always possible by grabbing the box's body.
    TrafoHandleT      m_DragState;      ///< Which of our handles (if any) is currently being dragged by the user.

    // This data is initialized in BeginTrafo() and used while a handle is being dragged.
    // It is independent of and commonly used by all trafo modes.
    AxesInfoT         m_DragAxes;       ///< The drag axes with which the drag was started and initialized.
    Vector3fT         m_LDownPosWorld;  ///< The mouse cursor position in world space when BeginTrafo() was called for beginning the drag.

    // Data for the specific transformations. Initialized in BeginTrafo(), then updated in UpdateTrafo() while the drag is active.
    Vector3fT         m_RefPos;         ///< A multi-purpose reference point in world space used for translations, scales and rotations.
    Vector3fT         m_Translate;      ///< The translation delta in world space as defined by the current drag operation.
    Vector3fT         m_Scale;          ///< The scale as defined by the current drag operation.
    float             m_RotAngle;       ///< The angle of rotation around m_RefPos as defined by the current drag operation. The axis of rotation is m_DragAxis.ThirdAxis.
    float             m_Shear;          ///< The amount of shear in world space as defined by the current drag operation. The direction of the shear depends on the handle that is being dragged.
};

#endif
