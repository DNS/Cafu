/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SCENEGRAPH_FACENODE_HPP_INCLUDED
#define CAFU_SCENEGRAPH_FACENODE_HPP_INCLUDED

#include "Node.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "Math3D/Polygon.hpp"

namespace MatSys
{
    class RenderMaterialT;
}

class  MaterialT;
struct FaceT;


namespace cf
{
    namespace SceneGraph
    {
        class FaceNodeT : public GenericNodeT
        {
            public:

            struct TexInfoT
            {
                Vector3fT U;
                Vector3fT V;
                float     OffsetU;
                float     OffsetV;
            };

            struct LightMapInfoT
            {
                unsigned short LightMapNr;  ///< In welcher LightMap liegt unsere Teil-LightMap?
                unsigned short PosS;        ///< S-Position unserer Teil-LightMap innerhalb der LightMap.
                unsigned short PosT;        ///< T-Position unserer Teil-LightMap innerhalb der LightMap.
                unsigned short SizeS;       ///< S-Größe unserer Teil-LightMap.
                unsigned short SizeT;       ///< T-Größe unserer Teil-LightMap.
            };

            struct SHLMapInfoT
            {
                unsigned short SHLMapNr;    ///< In welcher SHLMap liegt unsere Teil-SHLMap?
                unsigned short PosS;        ///< S-Position unserer Teil-SHLMap innerhalb der SHLMap.
                unsigned short PosT;        ///< T-Position unserer Teil-SHLMap innerhalb der SHLMap.
                unsigned short SizeS;       ///< S-Größe unserer Teil-SHLMap.
                unsigned short SizeT;       ///< T-Größe unserer Teil-SHLMap.
            };


            /// The constructor for creating an "empty" face.
            /// Needed e.g. by the named constructor CreateFromFile_cw() below.
            FaceNodeT(LightMapManT& LMM, SHLMapManT& SMM);

            /// Constructor for creating a FaceNodeT from components.
            /// This is currently only needed by the LoadWorld.cpp and LeakDetection.cpp files of the CaBSP tool.
            FaceNodeT(LightMapManT& LMM, SHLMapManT& SMM, const Polygon3T<double>& Poly_, MaterialT* Material_, const TexInfoT& TI_);

            /// The copy constructor.
            FaceNodeT(const FaceNodeT& Other);

            /// Named constructor.
            static FaceNodeT* CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, LightMapManT& LMM, SHLMapManT& SMM);

            /// The destructor.
            ~FaceNodeT();

            void InitRenderMeshesAndMats(const ArrayT<Vector3dT>& SharedVertices, const float LightMapPatchSize);

            /// Returns the DrawIndices. For CaBSP.
            ArrayT<unsigned long>& GetDrawIndices() { return DrawIndices; }

            /// Returns the LightMapMan. For CaBSP.
            LightMapManT& GetLightMapMan() const { return LightMapMan; }

            /// Returns the SHLMapMan. For CaBSP.
            SHLMapManT& GetSHLMapMan() const { return SHLMapMan; }

            /// Determines the lightmap color of this face near position Pos.
            /// @returns true if the lightmap color could be determined and written into the Red, Green and Blue parameters.
            bool GetLightmapColorNearPosition(const Vector3dT& Pos, Vector3fT& LightMapColor, const float LightMapPatchSize) const;

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


            // TODO: The stuff below should actually be protected or private rather than public.
            // However, it used to be in the obsolete FaceT struct that I used earlier, and a lot of Code (CaBSP, CaLight etc.)
            // still accesses our members directly. Should of course change that...
            public:

            static const double ROUND_EPSILON;      ///< The maximum amount that is allowed for geometry-related rounding errors.

            Polygon3T<double>        Polygon;
            MaterialT*               Material;      ///< The material that is assigned to this face. The pointer points into the MaterialManager-maintained list.
            TexInfoT                 TI;
            LightMapInfoT            LightMapInfo;
            SHLMapInfoT              SHLMapInfo;


            private:

            //void Init();    ///< Helper method for the constructors.
            //void Clean();   ///< Helper method for the destructor. Also called at the begin of Init().

            void operator = (const FaceNodeT&);     ///< Use of the Assignment Operator is not allowed.

            LightMapManT&            LightMapMan;
            SHLMapManT&              SHLMapMan;

            BoundingBox3T<double>    BB;            // TODO!!! This was in DrawableMapT, need to setup after ctor is complete!!!!!!!!!!!!!!!!!
            ArrayT<unsigned long>    DrawIndices;   // TODO!!! This was in MapT... !!!!!!!!!!!!!!!!!
            MatSys::MeshT            Mesh;          // TODO!!! This was in DrawableMapT, need to setup after ctor is complete!!!!!!!!!!!!!!!!!
            MatSys::RenderMaterialT* RenderMat;     // TODO!!! This was in DrawableMapT, need to setup after ctor is complete!!!!!!!!!!!!!!!!!
        };
    }
}

#endif
