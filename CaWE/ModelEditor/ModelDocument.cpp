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

#include "GuiSys/GuiImpl.hpp"
#include "Models/Loader_ase.hpp"
#include "Models/Loader_assimp.hpp"
#include "Models/Loader_cmdl.hpp"
#include "Models/Loader_fbx.hpp"
#include "Models/Loader_lwo.hpp"
#include "Models/Loader_md5.hpp"
#include "Models/Loader_mdl.hpp"
#include "String.hpp"

#include "wx/confbase.h"


ModelEditor::ModelDocumentT::SubmodelT::SubmodelT(const wxString& fn, CafuModelT* sm, const ArrayT<unsigned int>& jm)
    : m_Filename(fn), m_Submodel(sm), m_JointsMap(jm)
{
}


ModelEditor::ModelDocumentT::SubmodelT::~SubmodelT()
{
    delete m_Submodel;
}


static MapBrushT* GetGroundBrush(GameConfigT* GameConfig)
{
    EditorMaterialI* Mat =GameConfig->GetMatMan().FindMaterial(wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_Mat", "Textures/WilliH/rock01b"), true /*CreateDummy*/);
    const float      zPos=wxConfigBase::Get()->Read("ModelEditor/SceneSetup/GroundPlane_zPos", 0.0);
    const float      r   =400.0f;

    return MapBrushT::CreateBlock(BoundingBox3fT(Vector3fT(-r, -r, zPos-20.0f), Vector3fT(r, r, zPos)), Mat);
}


ModelEditor::ModelDocumentT::ModelDocumentT(GameConfigT* GameConfig, const wxString& FileName)
    : m_Model(LoadModel(FileName)),
      m_SequenceBB(m_Model->GetBB(-1, 0.0f)),
      m_Submodels(),
      m_Gui(new cf::GuiSys::GuiImplT("Win1=gui:new('WindowT'); gui:SetRootWindow(Win1); gui:activate(true); "
          "gui:setInteractive(true); gui:showMouse(true); Win1:set('rect', 0, 0, 640, 480); "
          "Win1:set('backColor', 150/255, 170/255, 204/255, 0.8); "
          "Win1:set('textAlignHor', 2); Win1:set('textAlignVer', 2); "
          "Win1:set('textColor', 15/255, 49/255, 106/255); "
          "Win1:set('text', 'This is a\\nfull-scale sample GUI.\\n\\nUse the Map Editor\\nto place the model and\\nto assign the true GUI.');", true)),
      m_Ground(GetGroundBrush(GameConfig)),
      m_GameConfig(GameConfig)
{
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

    for (unsigned long SmNr=0; SmNr<m_Submodels.Size(); SmNr++)
        delete m_Submodels[SmNr];

    for (unsigned long LsNr=0; LsNr<m_LightSources.Size(); LsNr++)
        delete m_LightSources[LsNr];

    for (unsigned long CamNr=0; CamNr<m_Cameras.Size(); CamNr++)
        delete m_Cameras[CamNr];

    delete m_Ground;
    delete m_Gui;
    delete m_Model;
}


void ModelEditor::ModelDocumentT::SetSelection(ModelElementTypeT Type, const ArrayT<unsigned int>& NewSel)
{
    wxASSERT(Type<4);
    m_Selection[Type]=NewSel;

    if (Type==ANIM)
    {
        if (m_Selection[ANIM].Size()==0)
        {
            m_SequenceBB=m_Model->GetBB(-1, 0.0f);
        }
        else
        {
            m_SequenceBB=BoundingBox3fT();

            for (unsigned long SelNr=0; SelNr<m_Selection[ANIM].Size(); SelNr++)
            {
                const CafuModelT::AnimT& Anim=m_Model->GetAnims()[m_Selection[ANIM][SelNr]];

                for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                    m_SequenceBB+=Anim.Frames[FrameNr].BB;
            }
        }
    }
}


void ModelEditor::ModelDocumentT::LoadSubmodel(const wxString& FileName)
{
    if (FileName=="") return;

    try
    {
        CafuModelT*          Submodel=LoadModel(FileName);
        ArrayT<unsigned int> JointsMap;

        for (unsigned int JointNr=0; JointNr<Submodel->GetJoints().Size(); JointNr++)
        {
            const std::string& SubName=Submodel->GetJoints()[JointNr].Name;

            unsigned int JNr;
            for (JNr=0; JNr<m_Model->GetJoints().Size(); JNr++)
                if (wxStricmp(SubName, m_Model->GetJoints()[JNr].Name)==0)
                    break;

            // Not found / no correspondence is indicated by a too large and thus invalid JNr.
            JointsMap.PushBack(JNr);
        }

        m_Submodels.PushBack(new SubmodelT(FileName, Submodel, JointsMap));
    }
    catch (const ModelT::LoadError& /*E*/)
    {
        // TODO: We really should have more detailed information about what exactly went wrong when loading the model...
        wxMessageBox(wxString("The submodel file \"")+FileName+"\" could not be loaded!", "Couldn't load or import submodel");
    }
    catch (const ModelLoaderT::LoadErrorT& LE)
    {
        wxMessageBox(wxString("The submodel file \"")+FileName+"\" could not be loaded:\n"+LE.what(), "Couldn't load or import submodel");
    }
}


void ModelEditor::ModelDocumentT::UnloadSubmodel(unsigned long SubmodelNr)
{
    if (SubmodelNr<m_Submodels.Size())
    {
        delete m_Submodels[SubmodelNr];
        m_Submodels.RemoveAtAndKeepOrder(SubmodelNr);
    }
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


namespace
{
    class ModelUserCallbacksT : public ModelLoaderT::UserCallbacksI
    {
        public:

        std::string GetPasswordFromUser(const std::string& Message, const std::string& Caption="Enter password")
        {
            return wxGetPasswordFromUser(Message, Caption).ToStdString();
        }
    };
}


/*static*/ CafuModelT* ModelEditor::ModelDocumentT::LoadModel(const wxString& FileName)
{
    const std::string   fn   =FileName.ToStdString();
    const int           Flags=ModelLoaderT::REMOVE_DEGEN_TRIANGLES | ModelLoaderT::REMOVE_UNUSED_VERTICES | ModelLoaderT::REMOVE_UNUSED_WEIGHTS;
    ModelUserCallbacksT UserCbs;

    // TODO: This duplicates the code in Model_proxy.cpp and should be combined elsewhere, e.g. into class ModelLoaderT.
    //       Better yet: Use the type system with the loaders, and be able to iterate over them.
         if (cf::String::EndsWith(fn, "3ds"    )) { LoaderFbxT    Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "ase"    )) { LoaderAseT    Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "cmdl"   )) { LoaderCafuT   Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "dae"    )) { LoaderFbxT    Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "dxf"    )) { LoaderFbxT    Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "fbx"    )) { LoaderFbxT    Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }
 // else if (cf::String::EndsWith(fn, "dlod"   )) return new ModelDlodT(fn);
    else if (cf::String::EndsWith(fn, "lwo"    )) { LoaderLwoT    Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "mdl"    )) { LoaderHL1mdlT Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "md5"    )) { LoaderMd5T    Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "md5mesh")) { LoaderMd5T    Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "obj"    )) { LoaderFbxT    Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }

    LoaderAssimpT Loader(fn, Flags);
    return new CafuModelT(Loader);
}
