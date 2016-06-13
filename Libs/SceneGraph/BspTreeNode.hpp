/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_SCENEGRAPH_BSPTREENODE_HPP_INCLUDED
#define CAFU_SCENEGRAPH_BSPTREENODE_HPP_INCLUDED

#include "Node.hpp"
#include "Math3D/Brush.hpp"
#include "Math3D/Polygon.hpp"

#if defined(_WIN32) && _MSC_VER<1600
#include "pstdint.h"            // Paul Hsieh's portable implementation of the stdint.h header.
#else
#include <stdint.h>
#endif


namespace cf
{
    namespace SceneGraph
    {
        class FaceNodeT;


        /// The class represents a BSP Tree node, implementing the Composite design pattern.
        /// BSP tree nodes are special group nodes that implement acceleration structures.
        class BspTreeNodeT : public GenericNodeT
        {
            public:

            struct NodeT
            {
                Plane3T<double> Plane;
                unsigned long   FrontChild;
                unsigned long   BackChild;
                bool            FrontIsLeaf;
                bool            BackIsLeaf;
            };


            struct LeafT
            {
                // These are just *indices* into the FaceChildren and OtherChildren arrays (that are "global" to the BSP tree) rather than *pointers* to
                // cf::SceneGraph::FaceNodeT and cf::SceneGraph::GenericNodeT because having the indices allows for a better implementation of some pieces of code,
                // e.g. when we want to keep track if one of the children has already been marked as being inside the PVS of a leaf.
                ArrayT<unsigned long>       FaceChildrenSet;
                ArrayT<unsigned long>       OtherChildrenSet;   ///< The contents of this leaf (bezier patches, terrains, etc. ... everything but faces).
                ArrayT< Polygon3T<double> > Portals;
                BoundingBox3T<double>       BB;
                bool                        IsInnerLeaf;
            };


            /// The constructor.
            /// Needed e.g. by the named constructor CreateFromFile_cw() below.
            BspTreeNodeT(float LightMapPatchSize, float SHLMapPatchSize);

            /// Named constructor.
            static BspTreeNodeT* CreateFromFile_cw(std::istream& InFile, aux::PoolT& Pool,
                LightMapManT& LMM, SHLMapManT& SMM, PlantDescrManT& PDM, const ArrayT<const TerrainT*>& ShTe, ModelManagerT& ModelMan);

            /// The destructor.
            ~BspTreeNodeT();

            float GetLightMapPatchSize() const { return m_LightMapPatchSize; }
            float GetSHLMapPatchSize() const { return m_SHLMapPatchSize; }

            void InitDrawing();

            /// Clips the line segment defined by P+U*Min and P+U*Max against the map and returns a value Hit such that
            /// P+U*Hit yields the point where the line segment intersects the BSP tree, with Min<=Hit<=Max.
            /// This is the classical "clip ray against BSP tree" implementation, used e.g. for PVS purposes:
            /// it works solely with the BSP tree and does not take "other" leaf contents etc. into account.
            /// This function MUST NOT BE CALLED ON AN EMPTY BSP TREE!
            /// The NodeNr and NodeIsLeaf parameters are for internal recursion only.
            /// They specify the recursion start and should always be omitted by the immediate caller.
            double ClipLine(const VectorT& P, const VectorT& U, double Min, double Max, unsigned long NodeNr=0, bool NodeIsLeaf=false) const;

            // The NodeT interface.
            void WriteTo(std::ostream& OutFile, aux::PoolT& Pool) const;
            const BoundingBox3T<double>& GetBoundingBox() const;

         // void InitDrawing();
            bool IsOpaque() const;
            void DrawAmbientContrib(const Vector3dT& ViewerPos) const;
            void DrawStencilShadowVolumes(const Vector3dT& LightPos, const float LightRadius) const;
            void DrawLightSourceContrib(const Vector3dT& ViewerPos, const Vector3dT& LightPos) const;
            void DrawTranslucentContrib(const Vector3dT& ViewerPos) const;


#if 0
            private:

            friend class Ca3DEWorld_EntityServiceInterfaceT;
            friend class BspTreeBuilderT;
#else
            // TODO!!!  This is currently made so that CaLight and CaSHL compile...
            public:
#endif

            /// In what leaf is 'Position' located? This function MUST NOT BE CALLED ON AN EMPTY MAP!
            unsigned long WhatLeaf(const VectorT& Position) const;

            /// In what leaves is 'BoundingBox' located? This function MUST NOT BE CALLED ON AN EMPTY MAP!
            /// The result will be APPENDED to the contents of the array 'ResultLeaves'.
            /// The third parameter is for recursion only and should always be omitted by the immediate caller.
            void WhatLeaves(ArrayT<unsigned long>& ResultLeaves, const BoundingBox3T<double>& BoundingBox, unsigned long NodeNr=0) const;

            /// This function traverses the BSP tree back-to-front, and stores the index numbers of the leaves it encounters in OrderedLeaves.
            /// The result is a permutation of the numbers 0...Leaves.Size()-1, such that a back-to-front sorting is obtained for the Origin.
            /// The cardinality of OrderedLeaves MUST MATCH THE CARDINALITY OF Leaves!
            void GetLeavesOrderedBackToFront(ArrayT<unsigned long>& OrderedFaces, const VectorT& Origin) const;

            /// Returns 'true' if leaf 'QueryLeafNr' is in the PVS of leaf 'LeafNr', and false otherwise. Do not call on empty map.
            bool IsInPVS(const unsigned long QueryLeafNr, unsigned long LeafNr) const;

            /// Returns 'true' if 'Position' is in the PVS of leaf 'LeafNr', and false otherwise. Do not call on empty map.
            bool IsInPVS(const VectorT& Position, unsigned long LeafNr) const;

            /// Returns 'true' if 'BoundingBox' is in or touches the PVS of leaf 'LeafNr', and false otherwise. Do not call on empty map.
            bool IsInPVS(const BoundingBox3T<double>& BoundingBox, unsigned long LeafNr) const;

            //void Init();    ///< Helper method for the constructors.
            //void Clean();   ///< Helper method for the destructor. Also called at the begin of Init().

            BspTreeNodeT(const BspTreeNodeT&);      ///< Use of the Copy    Constructor is not allowed.
            void operator = (const BspTreeNodeT&);  ///< Use of the Assignment Operator is not allowed.


            ArrayT<NodeT>                         Nodes;
            ArrayT<LeafT>                         Leaves;
            ArrayT<uint32_t>                      PVS;
            BoundingBox3T<double>                 BB;
            ArrayT<cf::SceneGraph::FaceNodeT*>    FaceChildren;     ///< The list of all the face  children of the BSP tree.
            ArrayT<cf::SceneGraph::GenericNodeT*> OtherChildren;    ///< The list of all the other children of the BSP tree.
            ArrayT<Vector3dT>                     GlobalDrawVertices;


            private:

            // Helper methods.
            void GetLeavesOrderedBackToFrontHelper(unsigned long NodeNr) const;
            void InitForNextLight() const;

            /*const*/ float m_LightMapPatchSize;  ///< The edge length of the lightmap patches. The same value is used for all child nodes in this BSP tree (or else CaLight would have to be costly updated).
            /*const*/ float m_SHLMapPatchSize;    ///< The edge length of the SHL map  patches. The same value is used for all child nodes in this BSP tree (or else CaLight would have to be costly updated).

            // Helper data for the Draw...() methods.
            mutable ArrayT<bool>                          FaceChildIsInViewerPVS;
            mutable ArrayT<bool>                          OtherChildIsInViewerPVS;
            mutable ArrayT<cf::SceneGraph::GenericNodeT*> BackToFrontListOpaque;
            mutable ArrayT<cf::SceneGraph::GenericNodeT*> BackToFrontListTranslucent;

            // Helper data for the Draw...() methods.
            mutable ArrayT<bool>                          FaceChildIsInLightSourcePVS;
            mutable ArrayT<bool>                          OtherChildIsInLightSourcePVS;
            mutable ArrayT<cf::SceneGraph::GenericNodeT*> FrontFacingList;
            mutable ArrayT<cf::SceneGraph::GenericNodeT*> BackFacingList;
            mutable bool                                  NextLightNeedsInit;
        };
    }
}

#endif
