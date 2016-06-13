/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MODEL_EDITOR_RENDERER_3D_HPP_INCLUDED
#define CAFU_MODEL_EDITOR_RENDERER_3D_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "wx/gdicmn.h"


namespace MatSys { class RenderMaterialT; }


namespace ModelEditor
{
    /// This class provides auxiliary means for rendering a 3D scene view of the model editor.
    /// It is similar to but much simpler than the 3D renderer of the map editor.
    /// In the future, we might split the Renderer3DT of the map editor into a GenericRenderer3DT base class and
    /// Renderer3DT derived classes for the map and model editor, for re-using the code of the shared functionality...
    class Renderer3DT
    {
        public:

        /// The constructor.
        Renderer3DT();

        /// The destructor.
        ~Renderer3DT();

        // Materials query methods.
        MatSys::RenderMaterialT* GetRMatWireframe()          const { return m_RMatWireframe;         }
        MatSys::RenderMaterialT* GetRMatWireframe_OffsetZ()  const { return m_RMatWireframeOZ;       }
        MatSys::RenderMaterialT* GetRMatFlatShaded()         const { return m_RMatFlatShaded;        }
        MatSys::RenderMaterialT* GetRMatFlatShaded_OffsetZ() const { return m_RMatFlatShadedOZ;      }
        MatSys::RenderMaterialT* GetRMatOverlay()            const { return m_RMatOverlay;           }
        MatSys::RenderMaterialT* GetRMatOverlay_OffsetZ()    const { return m_RMatOverlayOZ;         }
        MatSys::RenderMaterialT* GetRMatTerrainEditorTool()  const { return m_RMatTerrainEdit;       }
        MatSys::RenderMaterialT* GetRMatTerrainEyeDropper()  const { return m_RMatTerrainEyeDropper; }
        MatSys::RenderMaterialT* GetRMatWhite()              const { return m_RMatWhite; }
        MatSys::RenderMaterialT* GetRMatTexturedWireframe()  const { return m_RMatTexturedWireframe; }

        /// Returns a "shade" according to the direction of the given normal vector.
        float GetConstShade(const Vector3T<float>& Normal) const;

        /// Renders a box from the given bounding-box in the given color, with solid faces or in wireframe.
        void RenderBox(const BoundingBox3fT& BB, const wxColour& Color, bool Solid) const;

        /// Renders a box from the given eight vertices in the given color, with solid faces or in wireframe.
        /// The vertices are expected in the same order as given by the BoundingBox3T<T>::GetCornerVertices() method,
        /// and the box can be arbitrarily trans- or even deformed.
        void RenderBox(const Vector3fT Vertices[], const wxColour& Color, bool Solid) const;

        /// Renders a line from A to B in the given color.
        void RenderLine(const Vector3fT& A, const Vector3fT& B, const wxColour& Color) const;

        /// Renders the basis vectors (the "axes") of the given matrix at the given position with the given length.
        void BasisVectors(const Vector3fT& Pos, const cf::math::Matrix3x3fT& Mat, float Length=100.0f) const;

        /// Renders a cross-hair at the given point. Assumes that orthogonal rendering mode is active.
        void RenderCrossHair(const wxPoint& Center) const;


        private:

        Renderer3DT(const Renderer3DT&);            ///< Use of the Copy    Constructor is not allowed.
        void operator = (const Renderer3DT&);       ///< Use of the Assignment Operator is not allowed.

        MatSys::RenderMaterialT* m_RMatWireframe;           ///< The render material for wire-frame rendering.
        MatSys::RenderMaterialT* m_RMatWireframeOZ;         ///< The render material for wire-frame rendering (with polygon z-offset, e.g. for outlines).
        MatSys::RenderMaterialT* m_RMatFlatShaded;          ///< The render material for flat shaded (single solid color) rendering.
        MatSys::RenderMaterialT* m_RMatFlatShadedOZ;        ///< The render material for flat shaded (single solid color) rendering (with polygon z-offset, e.g. for decals).
        MatSys::RenderMaterialT* m_RMatOverlay;             ///< The render material for selection overlays (added in a second pass).
        MatSys::RenderMaterialT* m_RMatOverlayOZ;           ///< The render material for selection overlays (added in a second pass) (with polygon z-offset, e.g. for decals).
        MatSys::RenderMaterialT* m_RMatTerrainEdit;         ///< The render material overlay that is used to render the tool position in a terrain if the terrain edit tool is active.
        MatSys::RenderMaterialT* m_RMatTerrainEyeDropper;   ///< The render material overlay that is used to render the eyedropper tool position on a terrain.
        MatSys::RenderMaterialT* m_RMatWhite;               ///< This render material is intended as "debug" material for models in the Model Editor, so that the user can better inspect the surface lighting of the model.
        MatSys::RenderMaterialT* m_RMatTexturedWireframe;   ///< This render material is intended as "debug" material for models in the Model Editor, so that the user can better inspect the surface details of the model, e.g. at what vertex exactly are the GUI fixture points, etc.
    };
}

#endif
