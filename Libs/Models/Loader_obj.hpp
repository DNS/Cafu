/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

#ifndef _OBJ_MODEL_LOADER_HPP_
#define _OBJ_MODEL_LOADER_HPP_

#include "Loader.hpp"


/// This class imports a Wavefront OBJ model file into a new Cafu model.
class LoaderObjT : public ModelLoaderT
{
    public:

    struct ObjFaceVertexT
    {
        ObjFaceVertexT() : VertexNr(0), TexCoordNr(0), NormalNr(0) { }

        unsigned int VertexNr;      // 1-based index into the m_ObjVertices  array.
        unsigned int TexCoordNr;    // 1-based index into the m_ObjTexCoords array (0 indicates unknown/unspecified).
        unsigned int NormalNr;      // 1-based index into the m_ObjNormals   array (0 indicates unknown/unspecified).
    };

    struct ObjMeshT
    {
        std::string                      MtlName;
        ArrayT< ArrayT<ObjFaceVertexT> > Faces;
    };


    /// The constructor for importing a Wavefront OBJ model file into a new Cafu model.
    /// @param FileName   The name of the .obj file to import.
    LoaderObjT(const std::string& FileName) /*throw (ModelT::LoadError)*/;

    bool UseGivenTS() const;
    void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims);
    void Load(ArrayT<CafuModelT::GuiLocT>& GuiLocs) { }


    private:

    ArrayT<Vector3fT> m_ObjVertices;
    ArrayT<Vector3fT> m_ObjTexCoords;
    ArrayT<Vector3fT> m_ObjNormals;
    ArrayT<ObjMeshT>  m_ObjMeshes;
    unsigned int      m_NumShadedFaces[2];
};

#endif
