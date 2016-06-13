/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SCENEGRAPH_NODE_INTERFACE_HPP_INCLUDED
#define CAFU_SCENEGRAPH_NODE_INTERFACE_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"
#include "Math3D/Vector3.hpp"
#include "PatchMesh.hpp"

#include <fstream>


class PlantDescrManT;
class TerrainT;
class ModelManagerT;


namespace cf
{
    namespace SceneGraph
    {
        namespace aux
        {
            class PoolT;
        }

        class LightMapManT;
        class SHLMapManT;


        class GenericNodeT
        {
            public:

            /// Reads a GenericNodeT from InFile.
            static GenericNodeT* CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool,
                LightMapManT& LMM, SHLMapManT& SMM, PlantDescrManT& PDM, const ArrayT<const TerrainT*>& ShTe, ModelManagerT& ModelMan);

            /// The virtual destructor, so that derived classes can safely be deleted via a GenericNodeT (base class) pointer.
            virtual ~GenericNodeT() { }

            virtual void WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const
            {
            }

            /// Returns the bounding box of the contents of this scene node.
            virtual const BoundingBox3T<double>& GetBoundingBox() const
            {
                static BoundingBox3T<double> BB;

                return BB;
            }


         // virtual void InitDrawing()
         // {
         // }

            /// TODO / FIXME:
            /// This method is a hot-fix for getting the render order with translucent Bezier Patches right.
            /// It should be removed again and the whole system should be replaced with something as in the Q3 renderer!
            virtual bool IsOpaque() const
            {
                return true;
            }

            /// Draws the contents of this scene node.
            /// @param ViewerPos Position of the viewer.
            virtual void DrawAmbientContrib(const Vector3dT& ViewerPos) const
            {
            }

            // TODO: The signatures of all these Draw...() methods are highly questionable:
            //   The parameters (ViewerPos, LightPos, LightRadius) are also (in parallel) set in the MatSys::Renderer,
            //   LightRadius is given to this method but not to DrawLightSourceContrib, etc.
            //   See the implementation in BspTreeNode.cpp for why this is problematic.
            //   ===> The problem is probably best solved with the strategy mentioned above: Use something as in the Q3 renderer!
            virtual void DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const
            {
            }

            virtual void DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const
            {
            }

            virtual void DrawTranslucentContrib(const Vector3dT& ViewerPos) const
            {
            }


            /// If this NodeT uses lightmaps, this methods initializes default (full-bright) lightmaps for it at the proper size.
            /// This method is intended to be called by CaBSP, after it is completely done with changing the geometry of the node
            /// (the implementation would be part of the nodes constructor otherwise).
            /// The method may or may not use the LightMapManT for its purposes, the caller should not care.
            virtual void InitDefaultLightMaps(const float LightMapPatchSize)
            {
            }

            /// Creates the patch meshes for this NodeT for the purpose of radiosity computations (CaLight).
            /// The created patch meshes are just appended to the PatchMeshes array, i.e. the PatchMeshes array is not cleared by this function.
            /// The origins of the patches are chosen so that self-intersections due to rounding errors are impossible or at least unlikely.
            ///
            /// @param PatchMeshes    The array the created patch meshes are appended to. The array is not initially cleared by this function.
            /// @param SampleCoords   For each patch of each appended patch mesh, an array of sample coordinates is appended by this function.
            ///     The minimum number of samples returned per patch is normally one, namely the one identical to the patches coordinate.
            ///     NOTE: There may be NO SAMPLES AT ALL for a patch if InsideFace==false for that patch, so the caller must not make any assumptions.
            ///     The calling code may use that for example for computing initial sunlight information.
            ///     The array is not initially cleared by this function.
            /// @param LightMapPatchSize   The size of the lightmap patches.
            virtual void CreatePatchMeshes(ArrayT<PatchMeshT>& PatchMeshes, ArrayT< ArrayT< ArrayT<Vector3dT> > >& SampleCoords, const float LightMapPatchSize) const
            {
            }

            /// Takes the patches of the given patch mesh back into the lightmap of this node.
            virtual void BackToLightMap(const PatchMeshT& PatchMesh, const float LightMapPatchSize)
            {
            }
        };
    }
}

#endif
