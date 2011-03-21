/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _CA_MATSYS_MATERIAL_MANAGER_IMPLEMENTATION_HPP_
#define _CA_MATSYS_MATERIAL_MANAGER_IMPLEMENTATION_HPP_

#include "MaterialManager.hpp"

#include <map>


class TableT;


/// This class implements the MaterialManagerI interface.
class MaterialManagerImplT : public MaterialManagerI
{
    public:

    MaterialManagerImplT();
    ~MaterialManagerImplT();

    // The MaterialManagerI interface.
    ArrayT<MaterialT*> RegisterMaterialScript(const std::string& FileName, const std::string& BaseDir);
    ArrayT<MaterialT*> RegisterMaterialScriptsInDir(const std::string& DirName, const std::string& BaseDir, const bool Recurse=true);
 // const std::map<std::string, const MaterialT*>& GetAllMaterials();
    MaterialT* GetMaterial(const std::string& MaterialName);
 // void ClearAllMaterials();


    private:

    MaterialManagerImplT(const MaterialManagerImplT&);      // Use of the Copy Constructor    is not allowed.
    void operator = (const MaterialManagerImplT&);          // Use of the Assignment Operator is not allowed.

    ArrayT<std::string>               MaterialScriptFileNames;
    ArrayT<TableT*>                   Tables;
    std::map<std::string, MaterialT*> Materials;
};

#endif
