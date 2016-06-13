/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SCENEGRAPH_TERRAIN_HPP_INCLUDED
#define CAFU_SCENEGRAPH_TERRAIN_HPP_INCLUDED

#include "Node.hpp"


namespace MatSys
{
    class RenderMaterialT;
    class TextureMapI;
}


namespace cf
{
    namespace SceneGraph
    {
        class TerrainNodeT : public GenericNodeT
        {
            public:

            /// The constructor.
            TerrainNodeT();

            /// Constructor for creating a TerrainNodeT from parameters.
            /// @param BB_               The bounding box of the terrain node.
            /// @param Terrain_          The TerrainT instance to create the TerrainNodeT from.
            /// @param TerrainShareID_   Used for sharing common TerrainT instances across several TerrainNodeT's. (TODO: Needs better documentation!)
            /// @param MaterialName_     Name of the material that is applied to this terrain.
            /// @param LightMapPatchSize The size of the lightmap patches.
            TerrainNodeT(const BoundingBox3dT& BB_, const TerrainT& Terrain_, unsigned long TerrainShareID_, const std::string& MaterialName_, const float LightMapPatchSize);

            /// Named constructor.
            static TerrainNodeT* CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool, LightMapManT& LMM, SHLMapManT& SMM, const ArrayT<const TerrainT*>& ShTe);

            /// The destructor.
            ~TerrainNodeT();

            // The NodeT interface.
            void WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const;
            const BoundingBox3T<double>& GetBoundingBox() const;

         // void InitDrawing();
            bool IsOpaque() const;
            void DrawAmbientContrib(const Vector3dT& ViewerPos) const;
            void DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const;
            void DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const;
            void DrawTranslucentContrib(const Vector3dT& ViewerPos) const;


            private:

            void Init();    ///< Helper method for the constructors.
            void Clean();   ///< Helper method for the destructor. Also called at the begin of Init().

            TerrainNodeT(const TerrainNodeT&);      ///< Use of the Copy    Constructor is not allowed.
            void operator = (const TerrainNodeT&);  ///< Use of the Assignment Operator is not allowed.


            BoundingBox3T<double>    BB;                        ///< The lateral dimensions of the terrain.
            const TerrainT*          Terrain;                   ///< The actual terrain. The instance is kept outside of the scene graph, because it is shared with the physics world.
            unsigned long            TerrainShareID;            ///< The index of Terrain in the list of shared terrains.
            std::string              MaterialName;
            ArrayT<char>             LightMap;                  ///< The lightmap for this terrain. LightMap.Size()==3*p^2, where p is a power of 2 <= 256.
         // float                    LightMapTruePatchSize;     ///< The true patch size is obtained by (BB.Max.x-BB.Min.x)/p and (BB.Max.y-BB.Min.y)/p.
            ArrayT<float>            SHLMap;                    ///< The SHL map for this terrain. SHLMap.Size()==FaceT::SHLMapInfoT::NrOfBands^2 * q^2, where q is a power of 2 <= 256.
         // float                    SHLMapTruePatchSize;       ///< The true patch size is obtained by (BB.Max.x-BB.Min.x)/q and (BB.Max.y-BB.Min.y)/q.

            MatSys::RenderMaterialT* RenderMaterial;
            MatSys::TextureMapI*     LightMapTexture;
        };
    }
}

#endif
