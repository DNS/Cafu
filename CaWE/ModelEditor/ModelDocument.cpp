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
#include "../MapBrush.hpp"

#include "Models/Loader_ase.hpp"
#include "Models/Loader_assimp.hpp"
#include "Models/Loader_cmdl.hpp"
#include "Models/Loader_lwo.hpp"
#include "Models/Loader_md5.hpp"
#include "Models/Loader_mdl.hpp"
#include "String.hpp"

#include "wx/confbase.h"


static MapBrushT* GetGroundBrush(GameConfigT* GameConfig)
{
    EditorMaterialI* Mat =GameConfig->GetMatMan().FindMaterial(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_Mat", "Textures/WilliH/rock01b"), true /*CreateDummy*/);
    const float      zPos=wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_zPos", 0.0);
    const float      r   =400.0f;

    return MapBrushT::CreateBlock(BoundingBox3fT(Vector3fT(-r, -r, zPos-20.0f), Vector3fT(r, r, zPos)), Mat);
}


ModelEditor::ModelDocumentT::ModelDocumentT(GameConfigT* GameConfig, const wxString& ModelFileName)
    : m_Model(NULL),
      m_Ground(GetGroundBrush(GameConfig)),
      m_GameConfig(GameConfig)
{
    const std::string FileName=std::string(ModelFileName);  // Change to ModelFileName.ToStdString() with wx 2.9.1.

    // TODO: This duplicates the code in Model_proxy.cpp and should be combined elsewhere, e.g. into class ModelLoaderT.
         if (cf::String::EndsWith(FileName, "ase"    )) { LoaderAseT    Loader(FileName); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "cmdl"   )) { LoaderCafuT   Loader(FileName); m_Model=new CafuModelT(Loader); }
 // else if (cf::String::EndsWith(FileName, "dlod"   )) m_Model=new ModelDlodT(FileName);
    else if (cf::String::EndsWith(FileName, "mdl"    )) { LoaderHL1mdlT Loader(FileName); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "md5"    )) { LoaderMd5T    Loader(FileName); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "md5mesh")) { LoaderMd5T    Loader(FileName); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "lwo"    )) { LoaderLwoT    Loader(FileName); m_Model=new CafuModelT(Loader); }
    else
    {
        LoaderAssimpT Loader(FileName);
        m_Model=new CafuModelT(Loader);
    }

    m_Cameras.PushBack(new CameraT);
    m_Cameras[0]->Pos.y=-500.0f;

    m_LightSources.PushBack(new LightSourceT(true,  true, Vector3fT(200.0f,   0.0f, 200.0f), 1500.0f, wxColour(255, 235, 215)));
    m_LightSources.PushBack(new LightSourceT(false, true, Vector3fT(  0.0f, 200.0f, 200.0f), 1500.0f, wxColour(215, 235, 255)));
    m_LightSources.PushBack(new LightSourceT(false, true, Vector3fT(200.0f, 200.0f, 200.0f), 1500.0f, wxColour(235, 255, 215)));
}


ModelEditor::ModelDocumentT::~ModelDocumentT()
{
    for (unsigned long LsNr=0; LsNr<m_LightSources.Size(); LsNr++)
        delete m_LightSources[LsNr];

    for (unsigned long CamNr=0; CamNr<m_Cameras.Size(); CamNr++)
        delete m_Cameras[CamNr];

    delete m_Ground;
    delete m_Model;
}


void ModelEditor::ModelDocumentT::AdvanceTime(float Time)
{
    if (m_Anim.SequNr>=0 && Time*m_Anim.Speed!=0.0f)
    {
        m_Anim.FrameNr=m_Model->AdvanceFrameNr(m_Anim.SequNr, m_Anim.FrameNr, Time*m_Anim.Speed, m_Anim.Loop);

        // TODO: Update all observers...
    }
}


void ModelEditor::ModelDocumentT::SetNextAnimSequ()
{
    m_Anim.SequNr++;
    if (m_Anim.SequNr>=m_Model->GetNrOfSequences()) m_Anim.SequNr=-1;
    m_Anim.FrameNr=0.0f;

    // TODO: Update all observers...
}


void ModelEditor::ModelDocumentT::SetPrevAnimSequ()
{
    m_Anim.SequNr--;
    if (m_Anim.SequNr<-1) m_Anim.SequNr=m_Model->GetNrOfSequences()-1;
    m_Anim.FrameNr=0.0f;

    // TODO: Update all observers...
}


void ModelEditor::ModelDocumentT::SetAnimSpeed(float NewSpeed)
{
    m_Anim.Speed=NewSpeed;

    // TODO: Update all observers...
}
