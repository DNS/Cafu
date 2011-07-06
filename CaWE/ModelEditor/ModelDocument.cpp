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

#include "ModelDocument.hpp"
#include "../Camera.hpp"
#include "../EditorMaterialEngine.hpp"
#include "../GameConfig.hpp"
#include "../MapBrush.hpp"

#include "Models/Loader_ase.hpp"
#include "Models/Loader_assimp.hpp"
#include "Models/Loader_cmdl.hpp"
#include "Models/Loader_fbx.hpp"
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


class ModelUserCallbacksT : public ModelLoaderT::UserCallbacksI
{
    public:

    std::string GetPasswordFromUser(const std::string& Message, const std::string& Caption="Enter password")
    {
        return wxGetPasswordFromUser(Message, Caption).ToStdString();
    }
};


ModelEditor::ModelDocumentT::ModelDocumentT(GameConfigT* GameConfig, const wxString& ModelFileName)
    : m_Model(NULL),
      m_Ground(GetGroundBrush(GameConfig)),
      m_GameConfig(GameConfig)
{
    const std::string   FileName=std::string(ModelFileName);  // Change to ModelFileName.ToStdString() with wx 2.9.1.
    const int           Flags   =ModelLoaderT::REMOVE_DEGEN_TRIANGLES | ModelLoaderT::REMOVE_UNUSED_VERTICES | ModelLoaderT::REMOVE_UNUSED_WEIGHTS;
    ModelUserCallbacksT UserCbs;

    // TODO: This duplicates the code in Model_proxy.cpp and should be combined elsewhere, e.g. into class ModelLoaderT.
    //       Better yet: Use the type system with the loaders, and be able to iterate over them.
         if (cf::String::EndsWith(FileName, "3ds"    )) { LoaderFbxT    Loader(FileName, UserCbs, Flags); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "ase"    )) { LoaderAseT    Loader(FileName, Flags);          m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "cmdl"   )) { LoaderCafuT   Loader(FileName, Flags);          m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "dae"    )) { LoaderFbxT    Loader(FileName, UserCbs, Flags); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "dxf"    )) { LoaderFbxT    Loader(FileName, UserCbs, Flags); m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "fbx"    )) { LoaderFbxT    Loader(FileName, UserCbs, Flags); m_Model=new CafuModelT(Loader); }
 // else if (cf::String::EndsWith(FileName, "dlod"   )) m_Model=new ModelDlodT(FileName);
    else if (cf::String::EndsWith(FileName, "lwo"    )) { LoaderLwoT    Loader(FileName, Flags);          m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "mdl"    )) { LoaderHL1mdlT Loader(FileName, Flags);          m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "md5"    )) { LoaderMd5T    Loader(FileName, Flags);          m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "md5mesh")) { LoaderMd5T    Loader(FileName, Flags);          m_Model=new CafuModelT(Loader); }
    else if (cf::String::EndsWith(FileName, "obj"    )) { LoaderFbxT    Loader(FileName, UserCbs, Flags); m_Model=new CafuModelT(Loader); }
    else
    {
        LoaderAssimpT Loader(FileName, Flags);
        m_Model=new CafuModelT(Loader);
    }

    m_Cameras.PushBack(new CameraT);
    m_Cameras[0]->Pos.y=-500.0f;

    m_LightSources.PushBack(new LightSourceT(true,  true, Vector3fT(200.0f,   0.0f, 200.0f), 1500.0f, wxColour(255, 235, 215)));
    m_LightSources.PushBack(new LightSourceT(false, true, Vector3fT(  0.0f, 200.0f, 200.0f), 1500.0f, wxColour(215, 235, 255)));
    m_LightSources.PushBack(new LightSourceT(false, true, Vector3fT(200.0f, 200.0f, 200.0f), 1500.0f, wxColour(235, 255, 215)));

    // Init the editor materials.
    const std::map<std::string, MaterialT*>& ModelMaterials=m_Model->GetMaterialManager().GetAllMaterials();

    // No need to explicitly sort the m_EditorMaterials array after it has been filled in the order of the std::map.
    for (std::map<std::string, MaterialT*>::const_iterator It=ModelMaterials.begin(); It!=ModelMaterials.end(); It++)
        m_EditorMaterials.PushBack(new EngineMaterialT(It->second));
}


ModelEditor::ModelDocumentT::~ModelDocumentT()
{
    for (unsigned long MatNr=0; MatNr<m_EditorMaterials.Size(); MatNr++)
        delete m_EditorMaterials[MatNr];

    for (unsigned long LsNr=0; LsNr<m_LightSources.Size(); LsNr++)
        delete m_LightSources[LsNr];

    for (unsigned long CamNr=0; CamNr<m_Cameras.Size(); CamNr++)
        delete m_Cameras[CamNr];

    delete m_Ground;
    delete m_Model;
}


ArrayT<unsigned int> ModelEditor::ModelDocumentT::GetSelection_NextAnimSequ() const
{
    ArrayT<unsigned int> NextSel;
    const unsigned int   NextAnimSequNr=m_Selection[ANIM].Size()==0 ? 0 : m_Selection[ANIM][0]+1;

    if (NextAnimSequNr < m_Model->GetNrOfSequences())
        NextSel.PushBack(NextAnimSequNr);

    return NextSel;
}


ArrayT<unsigned int> ModelEditor::ModelDocumentT::GetSelection_PrevAnimSequ() const
{
    ArrayT<unsigned int> NextSel;
    const unsigned int   NextAnimSequNr=m_Selection[ANIM].Size()==0 ? m_Model->GetNrOfSequences()-1 : m_Selection[ANIM][0]-1;

    if (NextAnimSequNr < m_Model->GetNrOfSequences())
        NextSel.PushBack(NextAnimSequNr);

    return NextSel;
}


void ModelEditor::ModelDocumentT::AdvanceTime(float Time)
{
    if (m_Selection[ANIM].Size()>0 && Time*m_AnimState.Speed!=0.0f)
    {
        m_AnimState.FrameNr=m_Model->AdvanceFrameNr(m_Selection[ANIM][0], m_AnimState.FrameNr, Time*m_AnimState.Speed, m_AnimState.Loop);
    }
}


void ModelEditor::ModelDocumentT::SetAnimSpeed(float NewSpeed)
{
    m_AnimState.Speed=NewSpeed;
}
