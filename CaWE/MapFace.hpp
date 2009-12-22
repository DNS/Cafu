/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _MAP_FACE_HPP_
#define _MAP_FACE_HPP_

#include "SurfaceInfo.hpp"

#include "Math3D/BoundingBox.hpp"
#include "Math3D/Plane3.hpp"

#include <ostream>


class EditorMatManT;
class EditorMaterialI;
class MapBrushT;
class Renderer3DT;
namespace MatSys { class RenderMaterialT; }
class wxColour;


class MapFaceT
{
    public:

    MapFaceT(EditorMaterialI* Material=NULL);
    MapFaceT(EditorMaterialI* Material, const Plane3fT& Plane, const Vector3fT* PlanePoints, bool FaceAligned);

    // Named constructors for loading faces from map files.
    static MapFaceT Create_cmap(TextParserT& TP, EditorMatManT& MatMan);
    static MapFaceT Create_D3_map(TextParserT& TP, const Vector3fT& Origin, EditorMatManT& MatMan);
    static MapFaceT Create_HL1_map(TextParserT& TP, EditorMatManT& MatMan);
    static MapFaceT Create_HL2_vmf(TextParserT& TP, EditorMatManT& MatMan);


    void Save_cmap(std::ostream& OutFile) const;
    void Render3DBasic(Renderer3DT& Renderer, MatSys::RenderMaterialT* RenderMat, const wxColour& MeshColor, const int MeshAlpha) const;
    void Render3D(Renderer3DT& Renderer, const MapBrushT* ParentBrush) const;

    void SetMaterial(EditorMaterialI* Material);
    EditorMaterialI* GetMaterial() const { return m_Material; }

    void SetSurfaceInfo(const SurfaceInfoT& SI);
    const SurfaceInfoT& GetSurfaceInfo() const { return m_SurfaceInfo; }

    const Plane3T<float>&    GetPlane         () const { return m_Plane;          }
    const ArrayT<Vector3fT>& GetPlanePoints   () const { return m_PlanePoints;    }
    const ArrayT<Vector3fT>& GetVertices      () const { return m_Vertices;       }
 // const ArrayT<TexCoordT>& GetTextureCoords () const { return m_TextureCoords;  }     // Currently unused.
 // const ArrayT<TexCoordT>& GetLightmapCoords() const { return m_LightmapCoords; }     // Currently unused.

    // "Advanced" query methods, all constant.
    bool IsUVSpaceFaceAligned() const;    ///< Determines whether the uv-space of this face is  face-aligned. The uv-space of this face is  face-aligned when both UAxis and VAxis are orthogonal to m_Plane.Normal.
    bool IsUVSpaceWorldAligned() const;   ///< Determines whether the uv-space of this face is world-aligned. The uv-space of this face is world-aligned when both UAxis and VAxis are in the same principal plane.


    private:

    friend class MapBrushT;
    friend class EditSurfacePropsDialogT;   // The EditSurfacePropsDialogT is granted access, among others, to our m_IsSelected member (which is for use by the EditSurfacePropsDialogT only).

    void      UpdateTextureSpace();     ///< Uses the m_SurfaceInfo to re-calculate, for each vertex, the texture coords, the lightmap coords, the tangents and the bi-tangents.
    Vector3fT GetCenter() const;

    EditorMaterialI*  m_Material;       ///< The material that is applied to this face as specified in m_SurfaceInfo.
    SurfaceInfoT      m_SurfaceInfo;    ///< The surface info that specifies how the m_Material is applied to this face.
    Plane3T<float>    m_Plane;
    ArrayT<Vector3fT> m_PlanePoints;    ///< The three points that define the m_Plane.
    ArrayT<Vector3fT> m_Vertices;       ///< The vertices of the polygon that this face contributes to its brush.
    ArrayT<TexCoordT> m_TextureCoords;  ///< An array of texture  coordinates, one per face point.
    ArrayT<TexCoordT> m_LightmapCoords; ///< An array of lightmap coordinates, one per face point.
    ArrayT<Vector3fT> m_Tangents;
    ArrayT<Vector3fT> m_BiTangents;
    bool              m_IsSelected;     ///< Is this face selected? (For use by the "Edit Surface Properties" dialog only!)
};

#endif
