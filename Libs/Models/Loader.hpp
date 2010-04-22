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

#ifndef _MODEL_LOADER_HPP_
#define _MODEL_LOADER_HPP_

#include "Model_cmdl.hpp"


/// The base class for importing arbitrary model files into Cafu models.
/// The concrete derived classes determine which specific file format is imported.
class ModelLoaderT
{
    public:

    /// The constructor.
    ModelLoaderT(const std::string& FileName);

    /// Returns the file name of the imported model.
    const std::string& GetFileName() const { return m_FileName; }

    /// Some (static, non-animated) model file formats may bring all their tangent space data with them.
    /// For such files, there is no need for the Cafu model to recompute that data from the vertices.
    /// This method lets the Cafu model know whether the fixed, given tangent space should be used,
    /// or whether the Cafu model should recompute the tangent space (possibly after animation) itself.
    virtual bool UseGivenTS() const=0;

    /// Actually loads the file data into the approproate parts of the Cafu model.
    virtual void Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims)=0;


    protected:

    MaterialT* GetMaterialByName(const std::string& MaterialName) const;

    const std::string m_FileName;
};

#endif
