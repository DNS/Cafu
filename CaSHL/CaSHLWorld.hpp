/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

/****************************/
/*** CaSHL World (Header) ***/
/****************************/

#ifndef _CASHLWORLD_HPP_
#define _CASHLWORLD_HPP_

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

    CaSHLWorldT(const char* FileName, ModelManagerT& ModelMan);

    const cf::SceneGraph::BspTreeNodeT& GetBspTree() const { return *World.BspTree; }

    double TraceRay(const Vector3dT& Start, const Vector3dT& Ray) const;

    void PatchesToSHLMaps(const ArrayT< ArrayT<PatchT> >& Patches);

    // Forwarded functions.
    void SaveToDisk(const char* FileName) const;


    private:

    WorldT World;
};

#endif
