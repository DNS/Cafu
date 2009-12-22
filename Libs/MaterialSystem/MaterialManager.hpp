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

/**********************************/
/*** Material Manager Interface ***/
/**********************************/

#ifndef _CA_MATSYS_MATERIAL_MANAGER_INTERFACE_HPP_
#define _CA_MATSYS_MATERIAL_MANAGER_INTERFACE_HPP_

#include "Templates/Array.hpp"

#include <string>


class MaterialT;


/// This is an interface to the material manager.
/// The interface is specified as ABC in order to be able to share the material manager across exe/dll boundaries.
/// (Note that sharing across exe/dll boundaries is probably not needed: The MaterialManagerI is just a helper
///  for getting MaterialTs from script files. Thus we could also share (pointers to) arrays of MaterialTs directly.
///  (MODs should not register their own materials, as the engine registeres anything in the *Materials* dir. anyway.)
///  It is just the GetMaterial() "search" function that makes sharing the MatMan interesting.)
class MaterialManagerI
{
    public:

    // ************************************************************************************************
    // TODO: Should use   const MaterialT*   rather than just   MaterialT*   throughout this interface!
    // ************************************************************************************************

    /// Registers a material script file.
    /// The GetMaterial() function will look into the files registered here when looking for a material.
    /// @returns an array of pointers to all the new materials found in the cmat file FileName.
    ///          Note that only "new", not previously registered materials are considered here!
    virtual ArrayT<MaterialT*> RegisterMaterialScript(const std::string& FileName, const std::string& BaseDir)=0;

    /// Registers all ".cmat" files in a directory as material script files.
    /// @returns an array of pointers to all the new materials found in cmat files in DirName.
    ///          Note that only "new", not previously registered materials are considered here!
    virtual ArrayT<MaterialT*> RegisterMaterialScriptsInDir(const std::string& DirName, const std::string& BaseDir, const bool Recurse=true)=0;

    // This method is (currently) not needed.
    // /// Returns all the materials registered so far.
    // virtual const std::map<std::string, const MaterialT*>& GetAllMaterials()=0;

    /// Returns a material by its name.
    /// If the material is not found in the previously registered scripts, NULL is returned.
    virtual MaterialT* GetMaterial(const std::string& MaterialName)=0;

    // This method no longer exists - materials that have been registered once cannot be unregistered again without quitting the program.
    // This is to guarantee that the user code will never be left with stale MaterialT* pointers at any time.
    // /// Removes all materials from this material manager.
    // virtual void ClearAllMaterials()=0;

    /// Virtual destructor, so that nothing can go wrong and even g++ is happy.
    virtual ~MaterialManagerI() { }
};


/// A global pointer to the material manager that currently defaults to &MaterialManagerI::Get().
/// This pointer should be used by all code (e.g. models) that potentially is included in a DLL and should share the common Material Manager
/// of another module (e.g. the main executable). As the DLL will during its initialization reset this pointer to the one provided by the
/// executable, this goal is thus automatically achieved.
extern MaterialManagerI* MaterialManager;

#endif
