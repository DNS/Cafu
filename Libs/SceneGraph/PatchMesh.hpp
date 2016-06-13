/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_PATCH_MESH_HPP_INCLUDED
#define CAFU_PATCH_MESH_HPP_INCLUDED

class MaterialT;


namespace cf
{
    namespace SceneGraph
    {
        class GenericNodeT;
    }


    struct PatchT
    {
        VectorT UnradiatedEnergy;   ///< The energy that has not yet been radiated into the environment (per unit time per unit area).
        VectorT TotalEnergy;        ///< The energy that this patch radiates into the environment (per unit time per unit area).
        VectorT EnergyFromDir;      ///< The main (average) direction from which the total energy arrived at the patch.

        VectorT Coord;              ///< Position (+safety) in world space of the center of the patch. Valid only if InsideFace==true.
        VectorT Normal;             ///< The normal vector of the patch surface.
        double  Area;               ///< The area of the patch surface.
        bool    InsideFace;         ///< InsideFace==true <==> this patch is not completely outside its face.
    };

    struct PatchMeshT
    {
        unsigned long  Width;
        unsigned long  Height;
        ArrayT<PatchT> Patches;                 ///< The patches that form this patch mesh.

        bool           WrapsHorz;               ///< Returns whether the patch mesh wraps "in the width".  Note that if true, the extra-column at the right  is *not* included in the Patches array!
        bool           WrapsVert;               ///< Returns whether the patch mesh wraps "in the height". Note that if true, the extra-row    at the bottom is *not* included in the Patches array!

        const cf::SceneGraph::GenericNodeT* Node;       ///< The GenericNodeT this PatchMesh belongs to - quasi its "parent".
        MaterialT*                          Material;   ///< The MaterialT "beneath" this PatchMesh.


        /// Returns the const mesh patch at (i, j).
        const PatchT& GetPatch(unsigned long i, unsigned long j) const { assert(i<Width && j<Height); return Patches[j*Width+i]; }

        /// Returns the (non-const) mesh patch at (i, j).
        PatchT& GetPatch(unsigned long i, unsigned long j) { assert(i<Width && j<Height); return Patches[j*Width+i]; }
    };
}

#endif
