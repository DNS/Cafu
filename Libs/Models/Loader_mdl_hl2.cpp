/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#include "Loader_mdl_hl2.hpp"
#include "Loader_mdl_hl2.h"
#include "Math3D/Matrix3x3.hpp"
#include "String.hpp"

#include <cstdio>


namespace
{
    uint8_t* GetRawBytes(const std::string& BaseName, const std::string& Ext, ArrayT<uint8_t>& RawBytes)
    {
        FILE* InFile = fopen((BaseName + "." + Ext).c_str(), "rb");

        if (!InFile) throw ModelLoaderT::LoadErrorT("Could not fopen() the ." + Ext + " file of the model.");

        fseek(InFile, 0, SEEK_END); RawBytes.PushBackEmpty(ftell(InFile));
        fseek(InFile, 0, SEEK_SET);

        if (fread(&RawBytes[0], RawBytes.Size(), 1, InFile) == 0) { }   // Must check the return value of fread() with GCC 4.3...

        fclose(InFile);
        InFile = NULL;

        return &RawBytes[0];
    }
}


LoaderHL2mdlT::LoaderHL2mdlT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags |
          REMOVE_DEGEN_TRIANGLES |                          // Need this flag in order to pass all assertions in the CafuModelT code.
          REMOVE_UNUSED_VERTICES | REMOVE_UNUSED_WEIGHTS)   // The code below relies on postprocessing removing unused vertices and weights.
{
    // 1. Initialize auxiliary variables.
    // **********************************

    if (!cf::String::EndsWith(m_FileName, ".mdl")) throw LoadErrorT("HL2 model file name doesn't end with .mdl");

    const std::string BaseName(m_FileName.c_str(), m_FileName.length()-4);


    // 2. Load the model data.
    // ***********************

    // ...
}


void LoaderHL2mdlT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
}


void LoaderHL2mdlT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)
{
}
