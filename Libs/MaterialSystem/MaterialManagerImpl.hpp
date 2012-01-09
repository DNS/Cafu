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

    /// Returns whether the material with the given name is registered with the material manager,
    /// i.e.\ if a call to <code>GetMaterial(MaterialName)</code> will return successfully.
    /// Use this to avoid warning messages to the console if the material is not registered.
    bool HasMaterial(const std::string& MaterialName) const;

    // The MaterialManagerI interface.
    ArrayT<MaterialT*> RegisterMaterialScript(const std::string& FileName, const std::string& BaseDir);
    ArrayT<MaterialT*> RegisterMaterialScriptsInDir(const std::string& DirName, const std::string& BaseDir, const bool Recurse=true);
    const std::map<std::string, MaterialT*>& GetAllMaterials() const { return Materials; }
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
