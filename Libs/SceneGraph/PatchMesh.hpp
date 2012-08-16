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
