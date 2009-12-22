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

#ifndef _MODEL_LIGHTWAVE_OBJECT_HPP_
#define _MODEL_LIGHTWAVE_OBJECT_HPP_

#include "Model_md5.hpp"


/// This class represents a model in .lwo (LightWave Object) file format.
class ModelLwoT : public ModelMd5T
{
    public:

    /// The constructor for creating a .lwo model from a file.
    /// @param FileName   The name of the file to load the .lwo model from.
    ModelLwoT(const std::string& FileName) /*throw (ModelT::LoadError)*/;


    private:

    void ComputeTangents(const MeshT& Mesh, const unsigned long TriangleNr, Vector3fT& Tangent, Vector3fT& BiTangent) const;
};

#endif
