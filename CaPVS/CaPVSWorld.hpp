/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/****************************/
/*** CaPVS World (Header) ***/
/****************************/

#ifndef CAFU_CAPVSWORLD_HPP_INCLUDED
#define CAFU_CAPVSWORLD_HPP_INCLUDED

#include "../Common/World.hpp"


struct SuperLeafT
{
    struct NeighbourT
    {
        unsigned long     SuperLeafNr;      ///< Der Nachbar ist ein SuperLeaf, und hat die Indexnummer 'SuperLeafNr'.
        Polygon3T<double> SubPortal;        ///< Das zum Nachbar führende (Sub-)Portal.
    };

    ArrayT<unsigned long>       LeafSet;    ///< Die Menge der Map-Leaves, aus denen dieses SuperLeaf ursprünglich besteht.
    ArrayT< Polygon3T<double> > Portals;    ///< Die Portals dieses SuperLeafs, d.h. ALLE Portals ALLER Leaves des LeafSet.
    BoundingBox3T<double>       BB;         ///< Die BoundingBox der BoundingBoxes der Leaves im LeafSet.
    ArrayT<NeighbourT>          Neighbours; ///< Die Nachbarn dieses SuperLeafs.
};


class CaPVSWorldT
{
    private:

    WorldT                        m_World;
    cf::SceneGraph::BspTreeNodeT* m_BspTree;

    unsigned long SLC_MaxRecursionDepth;
    double        SLC_MinSubTreeFacesArea;

    double     SubTreeFacesArea                   (unsigned long NodeNr) const;
    bool       SuperLeafConditionIsMet            (unsigned long NodeNr, unsigned long RecursionDepth) const;
    void       CreateSuperLeafFromSubTreeRecursive(unsigned long NodeNr, SuperLeafT& SuperLeaf) const;
    SuperLeafT CreateSuperLeafFromSubTree         (unsigned long NodeNr) const;
    void       CreateSuperLeavesRecursive         (unsigned long NodeNr, ArrayT<SuperLeafT>& SuperLeaves, unsigned long RecursionDepth) const;


    public:

    // Constructor.
    CaPVSWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes, unsigned long SLC_MaxRecursionDepth_, double SLC_MinSubTreeFacesArea_);

    // This functions creates the 'SuperLeaves' from the world leaves. Notes:
    // a) There is no need at all to differentiate between inner and outer leaves.
    //    They are both treated the same, outer leaves just don't have any portals.
    // b) It is well possible that the resulting 'SuperLeaves' will not differ from the original 'Leaves',
    //    because in cases where the BSP tree depth is not limited, the end result is equivalent to the old, best-quality mode.
    void CreateSuperLeaves(ArrayT<SuperLeafT>& SuperLeaves) const;

    // This functions forwards the 'WorldT::MapT::WhatLeaf()' function.
    // It is useful, for example, if CaPVS wants to perform some trivial tests on visibility.
    unsigned long WhatLeaf(const VectorT& Position) const;

    // This functions forwards the 'WorldT::MapT::ClipLine()' function, NOT taking terrains into account.
    // It is useful, for example, if CaPVS wants to perform some trivial tests on visibility.
    double ClipLine(const VectorT& P, const VectorT& U) const;

    // Takes the 'SuperLeaves' and the 'SuperLeavesPVS', and constructs the leaf-wise world PVS from it.
    void StorePVS(const ArrayT<SuperLeafT>& SuperLeaves, const ArrayT<unsigned long>& SuperLeavesPVS);

    // Prints some statistics and returns a check-sum.
    unsigned long GetChecksumAndPrintStats() const;

    // Saves this world to disk.
    void SaveToDisk(const char* FileName) const;
};

#endif
