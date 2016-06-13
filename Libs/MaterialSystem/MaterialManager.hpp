/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/**********************************/
/*** Material Manager Interface ***/
/**********************************/

#ifndef CAFU_MATSYS_MATERIAL_MANAGER_INTERFACE_HPP_INCLUDED
#define CAFU_MATSYS_MATERIAL_MANAGER_INTERFACE_HPP_INCLUDED

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

    /// Returns whether the material with the given name is registered with the material manager,
    /// i.e.\ if a call to <code>GetMaterial(MaterialName)</code> will return successfully.
    /// Use this to avoid warning messages to the console if the material is not registered.
    virtual bool HasMaterial(const std::string& MaterialName) const=0;

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
