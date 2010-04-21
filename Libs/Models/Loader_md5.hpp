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

#ifndef _MD5_MODEL_LOADER_HPP_
#define _MD5_MODEL_LOADER_HPP_

#include "Loader.hpp"


/// This class imports a Doom3 (.md5) model file into a new Cafu model.
class LoaderMd5T : public ModelLoaderT
{
    public:

    /// The constructor for importing a Doom3 (.md5) file into a new Cafu model.
    /// @param FileName   The name of the .md5 file to import.
    /// If FileName ends with "md5", it is assumed that the file has a white-space separated list of one md5mesh and arbitrarily many md5anim files.
    /// If FileName ends with "md5mesh", this file is loaded without any animation information (e.g. for static detail models).
    LoaderMd5T(const std::string& FileName) /*throw (ModelT::LoadError)*/;

    bool UseGivenTS() const;
    void Load(CafuModelT* Model);
};

#endif
