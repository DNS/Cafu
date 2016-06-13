/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SCENEGRAPH_BEZIERPATCHNODE_HPP_INCLUDED
#define CAFU_SCENEGRAPH_BEZIERPATCHNODE_HPP_INCLUDED

#include "Node.hpp"


class MapBezierPatchT;

namespace MatSys
{
    class MeshT;
    class RenderMaterialT;
}


namespace cf
{
    namespace math
    {
        template<class T> class BezierPatchT;
    }


    namespace SceneGraph
    {
        class BezierPatchNodeT : public GenericNodeT
        {
            friend class ::MapBezierPatchT; // Editor needs access to the private meshdata of the bezier patch.


            public:

            /// The constructor for creating an "empty" bezier patch.
            /// Needed e.g. by the named constructor CreateFromFile_cw() below.
            BezierPatchNodeT(LightMapManT& LMM, float MaxError);

            /// Constructor for creating a BezierPatchNodeT from components.
            /// This is currently only needed by the LoadWorld.cpp file of the CaBSP tool.
            BezierPatchNodeT(LightMapManT& LMM, unsigned long SizeX_, unsigned long SizeY_, const ArrayT<float>& ControlPoints_, int SubdivsHorz_, int SubdivsVert_, MaterialT* Material_, float MaxError);

            /// Constructor for creating a BezierPatchNodeT from components.
            /// This is currently only needed by the MapBezierPatch.cpp file for patch rendering in CaWE.
            BezierPatchNodeT(LightMapManT& LMM, unsigned long SizeX_, unsigned long SizeY_, const ArrayT<Vector3fT>& ControlPointsXYZ_, const ArrayT<Vector3fT>& ControlPointsUV_, int SubdivsHorz_, int SubdivsVert_, MaterialT* Material_, float MaxError);

            /// Named constructor.
            static BezierPatchNodeT* CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, LightMapManT& LMM, SHLMapManT& SMM);

            /// The destructor.
            ~BezierPatchNodeT();

            /// Sets color for all meshes of this bezier patch.
            void UpdateMeshColor(const float red, const float green, const float blue, const float alpha);

            // The NodeT interface.
            void WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const;
            const BoundingBox3T<double>& GetBoundingBox() const;

         // void InitDrawing();
            bool IsOpaque() const;
            void DrawAmbientContrib(const Vector3dT& ViewerPos) const;
            void DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const;
            void DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const;
            void DrawTranslucentContrib(const Vector3dT& ViewerPos) const;

            void InitDefaultLightMaps(const float LightMapPatchSize);
            void CreatePatchMeshes(ArrayT<PatchMeshT>& PatchMeshes, ArrayT< ArrayT< ArrayT<Vector3dT> > >& SampleCoords, const float LightMapPatchSize) const;
            void BackToLightMap(const PatchMeshT& PatchMesh, const float LightMapPatchSize);


            private:

            struct LightMapInfoT
            {
                unsigned short LightMapNr;  ///< In welcher LightMap liegt unsere Teil-LightMap?
                unsigned short PosS;        ///< S-Position unserer Teil-LightMap innerhalb der LightMap.
                unsigned short PosT;        ///< T-Position unserer Teil-LightMap innerhalb der LightMap.
                unsigned short SizeS;       ///< S-Größe unserer Teil-LightMap.
                unsigned short SizeT;       ///< T-Größe unserer Teil-LightMap.
            };

            void Init();    ///< Helper method for the constructors.
            void Clean();   ///< Helper method for the destructor. Also called at the begin of Init().

            /// Automatically determines an appropriate number of subdivisions in horizontal direction,
            /// that is, the number of columns per 3x3 sub-patch (all 3x3 sub-patches get tesselated equally).
            /// Useful if SubdivsHorz is -1.
            unsigned long GetAutoSubdivsHorz() const;

            /// Automatically determines an appropriate number of subdivisions in vertical direction,
            /// that is, the number of rows per 3x3 sub-patch (all 3x3 sub-patches get tesselated equally).
            /// Useful if SubdivsVert is -1.
            unsigned long GetAutoSubdivsVert() const;

            /// Helper method, generates a mesh that is properly tesselated for lightmap purposes.
            void GenerateLightMapMesh(cf::math::BezierPatchT<float>& LightMapMesh, const float LightMapPatchSize, const bool ComputeTS) const;

            BezierPatchNodeT(const BezierPatchNodeT&);  ///< Use of the Copy    Constructor is not allowed.
            void operator = (const BezierPatchNodeT&);  ///< Use of the Assignment Operator is not allowed.

            // This is the basic definition of a patch, read from and written to disk.
            unsigned long     SizeX;            ///< Nr of columns.
            unsigned long     SizeY;            ///< Nr of rows.
            ArrayT<Vector3fT> ControlPointsXYZ; ///< The xyz spatial coordinates of the control points (SizeX*SizeY many).
            ArrayT<Vector3fT> ControlPointsUV;  ///< The uv  texture coordinates of the control points (SizeX*SizeY many).

            int               SubdivsHorz;      ///< Number of subdivisions in horizontal direction, or auto-detection if -1.
            int               SubdivsVert;      ///< Number of subdivisions in vertical   direction, or auto-detection if -1.
            /*const*/ float   m_MaxError;       ///< The maximal error distance for auto-subdividing bezier patches. Applies only if `SubdivsHorz` and `SubdivsVert` are -1.

            MaterialT*        Material;         ///< The material assigned to this patch.
            LightMapInfoT     LightMapInfo;     ///< The lightmap information for this patch.

            // Additional information for an entire patch (not only a 3x3 sub-patch).
            LightMapManT&            LightMapMan;
            BoundingBox3T<double>    BB;        ///< The BB of this bezier patch.
            ArrayT<MatSys::MeshT*>   Meshes;
            MatSys::RenderMaterialT* RenderMaterial;
        };
    }
}

#endif
