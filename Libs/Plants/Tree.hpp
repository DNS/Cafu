/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TREE_HPP_INCLUDED
#define CAFU_TREE_HPP_INCLUDED

#include "Templates/Array.hpp"
#include "Math3D/Vector3.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "Math3D/Matrix.hpp"
#include "Math3D/BoundingBox.hpp"

#include <string>


struct PlantDescriptionT;
namespace MatSys { class RenderMaterialT; }
class MaterialT;


/// Describes a renderable tree.
/// The tree is created from a PlantDescriptionT using a random seed so it will be randomly created
/// based on this plant description.
class TreeT
{
    private:

    /// The branch of a tree. A tree always consists of one or more branches, whereas the first branch is
    /// the trees trunk.
    struct BranchT
    {
        /// The segment of a branch. A branch consits of one or more segments.
        struct SegmentT
        {
            MatrixT       Matrix;               ///< Transformationsmatrix fuer dieses Segment.
            float         RadiusAtRoot;         ///< Radius des Querschnitts dieses Segments an seiner Wurzel (am "Anfang").
            unsigned long FirstCoordIndex;      ///< Index in the global tree vertices and texture coordinates array of the first vertice of this segment.
            unsigned long NrOfCoords;           ///< Number of vertices/texture coordinates of this segment.
        };

        /// The leaf of a tree.
        struct LeafT
        {
            Vector3fT                Origin   ; ///< Der Leaf-Ursprung/Mittelpunkt/Aufhaengepunkt (in World-Coords).
            float                    Coords[8]; ///< Rel. X/Y-Koord.paare bzgl. Origin fuer li. oben, re. oben, re. unten und li. unten (in World-Coords).
            float                    Color [3]; ///< Die Farbe (Farbe+Dimming+Varianz), mit der die Texture dieses Leafs moduliert werden soll.
            MatSys::RenderMaterialT* RenderMat; ///< The render material used to render this leaf.
        };


        float Length;                           ///< Laenge dieses Branches in Metern.
        float StartAngle;                       ///< Winkel der Abweichung von der Achse des Parents.
        float Radius;                           ///< Radius an der Wurzel.

        ArrayT<SegmentT>      Segments;         ///< Die Segmente dieses Branches.
        ArrayT<BranchT>       ChildBranches;    ///< Unsere Sub-Branches.
        ArrayT<unsigned long> LeavesIndices;    ///< Indizes unserer Blaetter ins 'AllLeaves'-Array (nuetzlich fuer LoD-Zwecke).
    };


    float                            Size;                ///< Height of the tree in world space units (millimeters).
    BranchT                          Trunk;               ///< The trees trunk with all of its branches.
    MatSys::RenderMaterialT*         BarkRenderMat;       ///< Render material used to render the trunk and all branches.
    ArrayT<MatSys::RenderMaterialT*> LeafRenderMats;      ///< The render materials used for the leaves.
    ArrayT<float>                    GlobalAbsCoordTable; ///< Array of all vertices (coordinates) of the trunk and all branches.
    ArrayT<float>                    GlobalTexCoordTable; ///< Array of the related texture coordinates.
    ArrayT<BranchT::LeafT>           AllLeaves;           ///< List of all leaves.
    BoundingBox3fT                   TreeBounds;

    BranchT ComputeBranch(float RelPosOnParent, char BranchLevel, const PlantDescriptionT* TD, const MatrixT& BranchMatrix, unsigned long& NumOfAllBranchTriangles);
    void    DrawBranch(const BranchT& Branch) const;
    void    GetLeavesAfterLoD(const BranchT& Branch, ArrayT<unsigned long>& LeavesAfterLoD, unsigned long& NrOfLeavesAfterLoD) const;
    void    DrawLeaf(const BranchT::LeafT& Leaf) const;


    public:

    bool  DrawLeaves;                ///< When 'true', also the leaves are drawn. When 'false', the leaves are omitted from drawing.
    bool  UseRealBlendingForLeaves;  ///< Determines if the leaves are drawn using real alpha blending, or only a simple alpha test.


    /// Default-Constructor.
    /// Not really useful, but we need it in order to be able to store 'TreeT' objects in an 'ArrayT'.
    TreeT() : BarkRenderMat(NULL) {}

    /// Creates a tree.
    /// @param TD The plant description used as a template for this tree.
    /// @param RandomSeed The random seed used to create this tree. Using a different random seed for every tree object will result
    ///        in different trees even if the same plant description is used.
    TreeT(const PlantDescriptionT* TD, unsigned long RandomSeed);

    /// Draws the tree.
    /// Note that the tree is drawn at (0, 0, 0) in modelspace so you have
    /// to translate the matrix to the right position before drawing the tree.
    void Draw() const;

    /// Returns the trees bounding box.
    const BoundingBox3fT& GetTreeBounds() const { return TreeBounds; }
};

#endif
