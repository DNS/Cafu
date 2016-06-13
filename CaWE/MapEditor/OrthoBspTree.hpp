/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_ORTHO_BSP_TREE_HPP_INCLUDED
#define CAFU_ORTHO_BSP_TREE_HPP_INCLUDED

#include "Math3D/BoundingBox.hpp"


class MapElementT;


/// This class represents an orthogonal BSP tree (axis-aligned split planes) that spatially organizes MapElementT instances.
///
/// A map element is generally linked in each leaf that it intersects, or instead whenever possible in the node closest to the
/// root where the element intersects *all* children.
/// Note that the latter is an optional feature that yields a tree that is logically equivalent to the pure "store all contents
/// in leaves" strategy (where nodes generally remain empty and store no contents at all), but in comparison saves some storage.
/// Also note the subtle difference to storing some contents in nodes (the elements that intersect a split plane) and some (the
/// rest) in leaves, like ancient version of our CaBSP: Keeping some contents in nodes like this would make it impossible to
/// determine the contents (the set of all map elements) that are inside a given node, because the set would be grossly oversized.
/// Our "extended leaves-only" approach saves both storage and is able to produce for each node the exact set of map elements
/// (required e.g. for view frustum tests).
///
/// Another feature of this implementation is that the bounding-box of a node is usually *not* the tight bounding-box over its
/// contents, but typically larger:
/// The bounding-box of the root node is the maximum bounds of the world, the bounding-box of its children is determined by
/// subdividing it along the split plane, etc. As such, the bounding-boxes in the tree only depend on the split planes, but not
/// directly on the contents of the nodes (and thus the BB member of the NodeT class is const).
///
/// Finally, note how these larger bounding-boxes affect the tree structure and size. There are two important differences:
/// 1. Where we would not have node planes at the outmost brush faces with a "tight" starting box, our approach definitively
///    comes up with these outmost walls as split planes sooner or later. This enlarges the tree, but these extra nodes are
///    fortunately not completely useless: When the user extends the map spatially, the extra nodes come in just handy.
/// 2. The center of our larger boxes is often different from the center of the tight bounding-boxes, as the tight boxes are
///    usually not symmetric to the origin. This causes different split planes to be chosen when the tree is built.
///    As with issue 1. though, this difference is not a principal or serious problem.
class OrthoBspTreeT
{
    public:

    class NodeT
    {
        public:

        /// As the nodes of the tree are not subdivided by arbitrary planes, but only by planes that are parallel
        /// to the major axes, we do not store a full Plane3T<> member with the nodes, but only a "compacted" representation:
        /// The type expresses which axes the plane is parallel to, the distance is the offset from the origin.
        enum PlaneTypeE
        {
            NONE=-1,    ///< No plane at all. Used for nodes that are actually leaves.
            ALONG_X,    ///< A plane with normal vector (1, 0, 0), parallel to the y- and z-axis.
            ALONG_Y,    ///< A plane with normal vector (0, 1, 0), parallel to the x- and z-axis.
            ALONG_Z     ///< A plane with normal vector (0, 0, 1), parallel to the x- and y-axis.
        };


        /// The constructor.
        /// @param BB   The bounding box of this node (one "half" of the box of the parent).
        NodeT(const BoundingBox3fT& BB);

        /// The destructor.
        ~NodeT();

        PlaneTypeE                  GetPlaneType() const { return m_PlaneType; }
        float                       GetPlaneDist() const { return m_PlaneDist; }
        const NodeT*                GetChild(unsigned int ChildNr) const { return m_Children[ChildNr]; }
        const ArrayT<MapElementT*>& GetElems() const { return m_Elems; }
        const BoundingBox3fT&       GetBB() const { return m_BB; }

        /// Returns the number of nodes in the (sub-)tree at and below this node.
        unsigned long GetNumNodes() const;

        /// Determines an axis-aligned split plane for further BSP partitioning of the contents of this node.
        /// For choosing the split plane, the method considers the bounding-box planes of all objects (polygons, brushes, terrains)
        /// of this node and all its ancestors, provided that they are wholly or partially in BB.
        /// When a split plane was found, the m_PlaneType and m_PlaneDist members are appropriately set and true is returned,
        /// otherwise they are initialized with NONE and 0, respectively, and the return value is false.
        /// @returns whether a split plane has successfully been determined.
        bool DetermineSplitPlane();

        /// Determines whether the given BB intersects (is partly inside) each child of this node.
        /// @param BB   The bounding box that is tested for intersection.
        bool IntersectsAllChildren(const BoundingBox3fT& BB) const;

        /// Finds any map elements in the (sub-)tree whose spatial position disagrees with their position in the tree.
        void FindMismatches(ArrayT<MapElementT*>& Mismatches) const;

        /// Recursively inserts the given element into the (sub-)tree at and below this node.
        void Insert(MapElementT* Elem);

        /// Removes the given element from the (sub-)tree at and below this node (the structure of the tree remains unchanged).
        void Remove(MapElementT* Elem);


        private:

        friend class OrthoBspTreeT;

        NodeT(const NodeT&);                ///< Use of the Copy    Constructor is not allowed.
        void operator = (const NodeT&);     ///< Use of the Assignment Operator is not allowed.

        PlaneTypeE           m_PlaneType;   ///< The type of the plane that subdivides this node (no plane at all, normal vector along the x-, y- or z-axis).
        float                m_PlaneDist;   ///< The distance of the plane to the origin. Corresponds to the Plane3fT::Dist member in a full plane.
        NodeT*               m_Parent;      ///< The parent of this node, NULL if this is the root node.
        NodeT*               m_Children[2]; ///< The children of this node at each side of the plane (NULL if there is no plane / the node is a leaf).
        ArrayT<MapElementT*> m_Elems;       ///< The list of map elements in this node.
        const BoundingBox3fT m_BB;          ///< The bounding-box of this node.
    };


    /// The constructor.
    OrthoBspTreeT(const ArrayT<MapElementT*>& Elems, const BoundingBox3fT& BB);

    /// The destructor.
    ~OrthoBspTreeT();

    /// Returns the BSP trees root node.
    const NodeT* GetRootNode() const { return m_RootNode; }

    /// Inserts the given element into the tree (the structure of the tree remains unchanged).
    void Insert(MapElementT* Elem) { m_RootNode->Insert(Elem); }

    /// Removes the given element from the tree (the structure of the tree remains unchanged).
    void Remove(MapElementT* Elem) { m_RootNode->Remove(Elem); }

    /// Removes and re-inserts map elements from and into the tree whose bounding-box changed so that it disagrees with the tree structure.
    /// This method is called once per "document frame" in ChildFrameT::OnIdle().
    /// @returns the number of map elements that have been updated.
    unsigned long Update();


    private:

    OrthoBspTreeT(const OrthoBspTreeT&);    ///< Use of the Copy    Constructor is not allowed.
    void operator = (const OrthoBspTreeT&); ///< Use of the Assignment Operator is not allowed.

    void BuildTree(NodeT* node);

    NodeT* m_RootNode;
};

#endif
