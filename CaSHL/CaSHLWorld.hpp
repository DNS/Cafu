/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/****************************/
/*** CaSHL World (Header) ***/
/****************************/

#ifndef CAFU_CASHLWORLD_HPP_INCLUDED
#define CAFU_CASHLWORLD_HPP_INCLUDED

#include "../Common/World.hpp"


// This switch controls the usage of normal-maps for SHL patches.
// Using normal-maps with SHL patches may or may not help, as explained in the appropriate code comments.
#define USE_NORMALMAPS 1


struct PatchT
{
    ArrayT<double> SHCoeffs_UnradiatedTransfer; // SH coefficients of the transfer function that is not yet "radiated" / "shot" into the environment.
    ArrayT<double> SHCoeffs_TotalTransfer;      // SH coefficients of the (total) transfer function (that is, direct and bounce transfer).

    VectorT       Coord;        // Position (+safety) in world space of the center of the patch. Valid only if InsideFace==true.
#if USE_NORMALMAPS
    VectorT       Normal;       // Normal of the patch. Derived from the normal map normals that are covered by this patch.
#endif
    bool          InsideFace;   // InsideFace==true <==> this patch is not completely outside its face
};


/// This class provides the interface that is needed to compute and store the SH lighting.
class CaSHLWorldT
{
    public:

    CaSHLWorldT(const char* FileName, ModelManagerT& ModelMan, cf::GuiSys::GuiResourcesT& GuiRes);

    const cf::SceneGraph::BspTreeNodeT& GetBspTree() const { return *m_BspTree; }

    double TraceRay(const Vector3dT& Start, const Vector3dT& Ray) const;

    void PatchesToSHLMaps(const ArrayT< ArrayT<PatchT> >& Patches);

    // Forwarded functions.
    void SaveToDisk(const char* FileName) const;


    private:

    WorldT                              m_World;
    const cf::SceneGraph::BspTreeNodeT* m_BspTree;
    cf::ClipSys::CollisionModelStaticT* m_CollModel;
};

#endif
