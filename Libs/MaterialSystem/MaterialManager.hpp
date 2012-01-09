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

/**********************************/
/*** Material Manager Interface ***/
/**********************************/

#ifndef _CA_MATSYS_MATERIAL_MANAGER_INTERFACE_HPP_
#define _CA_MATSYS_MATERIAL_MANAGER_INTERFACE_HPP_

#include "Templates/Array.hpp"

#include <map>
#include <string>


class MaterialT;


/// This is an interface to the material manager.
/// A material manager keeps a set of materials and their related script state, and finds materials by name.
///
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

    /// Returns all the materials registered so far.
    virtual const std::map<std::string, MaterialT*>& GetAllMaterials() const=0;

    /// Returns a material by its name.
    /// If the material is not found in the previously registered scripts, NULL is returned.
    virtual MaterialT* GetMaterial(const std::string& MaterialName) const=0;

    // This method no longer exists - materials that have been registered once cannot be unregistered again without quitting the program.
    // This is to guarantee that the user code will never be left with stale MaterialT* pointers at any time.
    // /// Removes all materials from this material manager.
    // virtual void ClearAllMaterials()=0;

    /// Virtual destructor, so that nothing can go wrong and even g++ is happy.
    virtual ~MaterialManagerI() { }
};


/// A global pointer to an implementation of the MaterialManagerI interface.
///
/// Each module (exe or dll) that uses this pointer must somewhere provide exactly one definition for it (none is provided by the MatSys).
/// That is, typically the main.cpp or similar file of each exe and dll must contain a line like
///     MaterialManagerI* MaterialManager=NULL;
/// or else the module will not link successfully due to an undefined symbol.
///
/// Exe files will then want to reset this pointer to an instance of a MaterialManagerImplT during their initialization
/// e.g. by code like:   MaterialManager=new MaterialManagerImplT;
/// Note that the MaterialManagerImplT ctor may require that other interfaces (e.g. the Console) have been inited first.
///
/// Dlls typically get one of their init functions called immediately after they have been loaded.
/// By doing so, the exe passes a pointer to its above instance to the dll, which in turn copies it to its MaterialManager variable.
extern MaterialManagerI* MaterialManager;

#endif
