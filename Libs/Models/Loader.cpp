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

#include "Loader.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/ConVar.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/MaterialManager.hpp"


ModelLoaderT::ModelLoaderT(const std::string& FileName)
    : m_FileName(FileName)
{
}


MaterialT* ModelLoaderT::GetMaterialByName(const std::string& MaterialName) const
{
    MaterialT* Material=MaterialManager->GetMaterial(MaterialName);

    if (Material==NULL)
    {
        static ConVarT ModelReplacementMaterial("modelReplaceMat", "meta/model_replacement", 0, "Replacement for unknown materials of models.", NULL);

        Console->Warning("Model \""+m_FileName+"\" refers to unknown material \""+MaterialName+"\". "
            "Replacing the material with \""+ModelReplacementMaterial.GetValueString()+"\".\n");

        Material=MaterialManager->GetMaterial(ModelReplacementMaterial.GetValueString());
    }

    if (Material==NULL)
    {
        Console->Warning("The replacement material is also unknown - the model will NOT render!\n");
    }

    if (Material!=NULL && !Material->LightMapComp.IsEmpty())
    {
        Console->Warning("Model \""+m_FileName+"\" uses material \""+MaterialName+"\", which in turn has lightmaps defined.\n"
            "It will work in the ModelViewer, but for other applications like Cafu itself you should use a material without lightmaps.\n");
            // It works in the ModelViewer because the ModelViewer is kind enough to provide a default lightmap...
    }

    return Material;
}
