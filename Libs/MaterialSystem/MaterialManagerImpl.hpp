/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/***************************************/
/*** Material Manager Implementation ***/
/***************************************/

#ifndef CAFU_MATSYS_MATERIAL_MANAGER_IMPLEMENTATION_HPP_INCLUDED
#define CAFU_MATSYS_MATERIAL_MANAGER_IMPLEMENTATION_HPP_INCLUDED

#include "MaterialManager.hpp"

#include <map>


class TableT;


/// This class implements the MaterialManagerI interface.
class MaterialManagerImplT : public MaterialManagerI
{
    public:

    MaterialManagerImplT();
    ~MaterialManagerImplT();

    /// Registers a copy of the given material \c Mat and returns a pointer to the registered copy.
    /// If a material with the same name \c Mat.Name is already registered with the material manager,
    /// \c Mat is not registered, but a pointer to the already registered instance with the same name is returned
    /// (use HasMaterial() in advance to unambigiously distingush between the two cases).
    ///
    /// @param Mat   The material of which a copy is to be registered with the material manager.
    /// @returns A pointer to the registered material instance (or a previously existing instance with the same name).
    MaterialT* RegisterMaterial(const MaterialT& Mat);


    // The MaterialManagerI interface.
    ArrayT<MaterialT*> RegisterMaterialScript(const std::string& FileName, const std::string& BaseDir);
    ArrayT<MaterialT*> RegisterMaterialScriptsInDir(const std::string& DirName, const std::string& BaseDir, const bool Recurse=true);
    const std::map<std::string, MaterialT*>& GetAllMaterials() const { return Materials; }
    bool HasMaterial(const std::string& MaterialName) const;
    MaterialT* GetMaterial(const std::string& MaterialName) const;
 // void ClearAllMaterials();


    private:

    MaterialManagerImplT(const MaterialManagerImplT&);      // Use of the Copy Constructor    is not allowed.
    void operator = (const MaterialManagerImplT&);          // Use of the Assignment Operator is not allowed.

    ArrayT<std::string>               MaterialScriptFileNames;
    ArrayT<TableT*>                   Tables;
    std::map<std::string, MaterialT*> Materials;
};

#endif
