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

#include "ModelDocument.hpp"
#include "../GameConfig.hpp"

#include "Models/Loader_ase.hpp"
#include "Models/Loader_lwo.hpp"
#include "Models/Loader_md5.hpp"
#include "String.hpp"


ModelEditor::ModelDocumentT::ModelDocumentT(GameConfigT* GameConfig, const wxString& ModelFileName)
    : m_Model(NULL),
      m_GameConfig(GameConfig)
{
    const std::string FileName=std::string(ModelFileName);  // Change to ModelFileName.ToStdString() with wx 2.9.1.

    // TODO: This duplicates the code in Model_proxy.cpp and should be combined elsewhere, e.g. into class ModelLoaderT.
         if (cf::String::EndsWith(FileName, "ase"    )) { LoaderAseT Loader(FileName); m_Model=new CafuModelT(Loader); }
 // else if (cf::String::EndsWith(FileName, "dlod"   )) m_Model=new ModelDlodT(FileName);
 // else if (cf::String::EndsWith(FileName, "mdl"    )) m_Model=new ModelMdlT (FileName);
    else if (cf::String::EndsWith(FileName, "md5"    )) { LoaderMd5T Loader(FileName); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "md5mesh")) { LoaderMd5T Loader(FileName); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "md5anim")) { LoaderMd5T Loader(FileName); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "lwo"    )) { LoaderLwoT Loader(FileName); m_Model=new CafuModelT(Loader); }
    else throw ModelT::LoadError();
}


ModelEditor::ModelDocumentT::~ModelDocumentT()
{
    delete m_Model;
}
