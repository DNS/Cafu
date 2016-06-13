/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_RENDERER_3D_HPP_INCLUDED
#define CAFU_RENDERER_3D_HPP_INCLUDED

#include "ChildFrameViewWin.hpp"
#include "OrthoBspTree.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Math3D/Plane3.hpp"
#include "Templates/Array.hpp"

#include "wx/gdicmn.h"


class MapElementT;
class MapDocumentT;
class ToolT;
class ViewWindow3DT;
namespace MatSys { class RenderMaterialT; }


/// This class provides auxiliary means for rendering a 3D view.
/// A 3D view is essentially rendered by calling the MapElementT::Render3D() method of all relevant (visible) MapElementTs
/// within the document, see the ViewWindow3DT::OnPaint() method for details. The map elements can render themselves either
/// directly by means of the Cafu MatSys, or by calls to the auxiliary functions in this class.
class Renderer3DT
{
    public:

    /// A helper class that temporarily sets up the matrices in the Cafu MatSys for orthogonal rendering into the given 3D view window.
    /// Just create an instance of this class on the stack, and within the current scope, the orthogonal mode will be active.
    /// It works by setting the orthogonal matrices in the constructor and restoring the original matrices in the destructor.
    class UseOrthoMatricesT
    {
        public:

        /// The constructor.
        UseOrthoMatricesT(const wxWindow& Window);

        /// The destructor.
        ~UseOrthoMatricesT();
    };


    /// The constructor.
    Renderer3DT(ViewWindow3DT& ViewWin3D);

    /// The destructor.
    ~Renderer3DT();

    /// Initializes the rendering of a new frame by computing and caching all relevant data.
    void InitFrame();

    // Methods for getting the renderer state.
    const ViewWindow3DT&        GetViewWin3D() const { return m_ViewWin3D; }
    const Plane3fT*             GetViewFrustumPlanes() const { return m_FrustumPlanesCache; }
    const ArrayT<MapElementT*>& GetVisElemsBackToFront() const { return m_VisElemsBackToFront; }
    float                       GetConstShade(const Vector3T<float>& Normal) const;

    // Materials query methods.
    MatSys::RenderMaterialT* GetRMatWireframe()          const { return m_RMatWireframe;         }
    MatSys::RenderMaterialT* GetRMatWireframe_OffsetZ()  const { return m_RMatWireframeOZ;       }
    MatSys::RenderMaterialT* GetRMatFlatShaded()         const { return m_RMatFlatShaded;        }
    MatSys::RenderMaterialT* GetRMatFlatShaded_OffsetZ() const { return m_RMatFlatShadedOZ;      }
    MatSys::RenderMaterialT* GetRMatOverlay()            const { return m_RMatOverlay;           }
    MatSys::RenderMaterialT* GetRMatOverlay_OffsetZ()    const { return m_RMatOverlayOZ;         }
    MatSys::RenderMaterialT* GetRMatTerrainEditorTool()  const { return m_RMatTerrainEdit;       }
    MatSys::RenderMaterialT* GetRMatTerrainEyeDropper()  const { return m_RMatTerrainEyeDropper; }

    /// Renders a box from the given bounding-box in the given color, with solid faces or in wireframe.
    void RenderBox(const BoundingBox3fT& BB, const wxColour& Color, bool Solid) const;

    /// Renders a box from the given eight vertices in the given color, with solid faces or in wireframe.
    /// The vertices are expected in the same order as given by the BoundingBox3T<T>::GetCornerVertices() method,
    /// and the box can be arbitrarily trans- or even deformed.
    void RenderBox(const Vector3fT Vertices[], const wxColour& Color, bool Solid) const;

    /// Renders a line from A to B in the given color.
    void RenderLine(const Vector3fT& A, const Vector3fT& B, const wxColour& Color) const;

    /// Renders the split planes of the BSP tree at and below the given node, up to the given depth.
    void RenderSplitPlanes(const OrthoBspTreeT::NodeT* Node, int Depth) const;

    /// Renders the basis vectors (the "axes") of the given matrix at the given position with the given length.
    void BasisVectors(const Vector3fT& Pos, const cf::math::Matrix3x3fT& Mat, float Length=100.0f) const;

    /// Renders a cross-hair at the given point. Assumes that orthogonal rendering mode is active.
    void RenderCrossHair(const wxPoint& Center) const;


    private:

    /// An enumeration of locations of a bounding-box in relation to the view frustum.
    enum RelLocT
    {
        COMPL_OUTSIDE,
        COMPL_INSIDE,
        INTERSECTS
    };

    Renderer3DT(const Renderer3DT&);            ///< Use of the Copy    Constructor is not allowed.
    void operator = (const Renderer3DT&);       ///< Use of the Assignment Operator is not allowed.

    RelLocT RelFrustum(const BoundingBox3fT& BB) const;
    void    GetRenderList(const OrthoBspTreeT::NodeT* Node, RelLocT ParentLoc);

    ViewWindow3DT&           m_ViewWin3D;               ///< The 3D view window that owns this renderer / that this renderer is assigned to.
    const ToolT*             m_ActiveToolCache;         ///< Caches the active tool pointer during a call to InitFrame(). NULL at all other times.
    Plane3fT                 m_FrustumPlanesCache[6];   ///< Caches the six planes that define the current view frustum for the current frame.
    ArrayT<MapElementT*>     m_VisElemsBackToFront;     ///< Used during rendering, the back-to-front ordered list of map elements that are in the view frustum and visible is build and kept here.

    MatSys::RenderMaterialT* m_RMatWireframe;           ///< The render material for wire-frame rendering.
    MatSys::RenderMaterialT* m_RMatWireframeOZ;         ///< The render material for wire-frame rendering (with polygon z-offset, e.g. for outlines).
    MatSys::RenderMaterialT* m_RMatFlatShaded;          ///< The render material for flat shaded (single solid color) rendering.
    MatSys::RenderMaterialT* m_RMatFlatShadedOZ;        ///< The render material for flat shaded (single solid color) rendering (with polygon z-offset, e.g. for decals).
    MatSys::RenderMaterialT* m_RMatOverlay;             ///< The render material for selection overlays (added in a second pass).
    MatSys::RenderMaterialT* m_RMatOverlayOZ;           ///< The render material for selection overlays (added in a second pass) (with polygon z-offset, e.g. for decals).
    MatSys::RenderMaterialT* m_RMatTerrainEdit;         ///< The render material overlay that is used to render the tool position in a terrain if the terrain edit tool is active.
    MatSys::RenderMaterialT* m_RMatTerrainEyeDropper;   ///< The Render material overlay that is used to render the eyedropper tool position on a terrain.
};

#endif
