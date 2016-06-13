/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "ModelDocument.hpp"
#include "../Camera.hpp"
#include "../EditorMaterialEngine.hpp"
#include "../GameConfig.hpp"
#include "../MapEditor/MapBrush.hpp"

#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "Models/Loader_ase.hpp"
#include "Models/Loader_cmdl.hpp"
#include "Models/Loader_dlod.hpp"
#include "Models/Loader_dummy.hpp"
#include "Models/Loader_fbx.hpp"
#include "Models/Loader_lwo.hpp"
#include "Models/Loader_md5.hpp"
#include "Models/Loader_mdl.hpp"
#include "String.hpp"
#include "UniScriptState.hpp"

#include "wx/confbase.h"


ModelEditor::ModelDocumentT::AnimStateT::AnimStateT(const CafuModelT& Model)
    : LastStdAE(Model.GetAnimExprPool().GetStandard(-1, 0.0f)),
      Pose(Model, LastStdAE),
      Speed(1.0f),
      Loop(true)
{
}


ModelEditor::ModelDocumentT::SubmodelT::SubmodelT(CafuModelT* Submodel)
    : m_Submodel(Submodel),
      m_Pose(*Submodel, Submodel->GetAnimExprPool().GetStandard(-1, 0.0f))
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
      m_AnimState(*m_Model),
      m_SequenceBB(m_AnimState.Pose.GetBB()),
      m_Submodels(),
      m_ScriptState(NULL),
      m_Gui(NULL),
      m_Ground(GetGroundBrush(GameConfig)),
      m_GameConfig(GameConfig)
{
    m_ScriptState = new cf::UniScriptStateT();
    cf::GuiSys::GuiImplT::InitScriptState(*m_ScriptState);

    m_Gui = new cf::GuiSys::GuiImplT(
        *m_ScriptState,
        GameConfig->GetGuiResources());

    m_Gui->LoadScript(
        "local gui = ...\n"
        "local Root = gui:new('WindowT', 'Root')\n"
        "gui:SetRootWindow(Root)\n"
        "\n"
        "function Root:OnInit()\n"
        "    self:GetTransform():set('Pos', 0, 0)\n"
        "    self:GetTransform():set('Size', 640, 480)\n"
        "\n"
        "    local c1 = gui:new('ComponentTextT')\n"
        "    c1:set('Text', 'This is a\\nfull-scale sample GUI.\\n\\nUse the Map Editor\\nto place the model and\\nto assign the true GUI.')\n"
        " -- c1:set('Font', 'Fonts/Impact')\n"
        "    c1:set('Scale', 0.6)\n"
        "    c1:set('Padding', 0, 0)\n"
        "    c1:set('Color', 15/255, 49/255, 106/255)\n"
        " -- c1:set('Alpha', 0.5)\n"
        "    c1:set('horAlign', 0)\n"
        "    c1:set('verAlign', 0)\n"
        "\n"
        "    local c2 = gui:new('ComponentImageT')\n"
        "    c2:set('Material', '')\n"
        "    c2:set('Color', 150/255, 170/255, 204/255)\n"
        "    c2:set('Alpha', 0.8)\n"
        "\n"
        "    self:AddComponent(c1, c2)\n"
        "\n"
        "    gui:activate      (true)\n"
        "    gui:setInteractive(true)\n"
        "    gui:showMouse     (false)\n"
        "    gui:setFocus      (Root)\n"
        "end\n",
        cf::GuiSys::GuiImplT::InitFlag_InlineCode);

    m_Cameras.PushBack(new CameraT);
    m_Cameras[0]->Pos.y=-500.0f;
    m_Cameras[0]->NearPlaneDist=1.0f;

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
    m_Ground = NULL;

    m_Gui = NULL;

    delete m_ScriptState;
    m_ScriptState = NULL;

    delete m_Model;
    m_Model = NULL;
}


int ModelEditor::ModelDocumentT::GetSelSkinNr() const
{
    return m_Selection[SKIN].Size()==0 ? -1 : m_Selection[SKIN][0];
}


wxString ModelEditor::ModelDocumentT::GetSelSkinString() const
{
    if (m_Selection[SKIN].Size()==0)
        return "default skin";

    return wxString::Format("skin %u: \"%s\"", m_Selection[SKIN][0], m_Model->GetSkins()[m_Selection[SKIN][0]].Name);
}


IntrusivePtrT<const cf::GuiSys::GuiImplT> ModelEditor::ModelDocumentT::GetGui() const
{
    return m_Gui;
}


void ModelEditor::ModelDocumentT::SetSelection(ModelElementTypeT Type, const ArrayT<unsigned int>& NewSel)
{
    wxASSERT(Type<6);
    m_Selection[Type]=NewSel;

    if (Type==ANIM)
    {
        IntrusivePtrT<AnimExpressionT> BlendFrom=m_AnimState.Pose.GetAnimExpr();

        // If BlendFrom is a blend anim expression that has not yet begun (Frac is still 0),
        // this means that we got another call to SetSelection() immediately before this one.
        // This can happen when the user clicked on a new animation in the animations list,
        // which (when CTRL is not used) issues two calls, one for the deselection of the previous
        // anim (causing us to blend to the -1 sequence), and one for the selection of the new.
        // In this case, just skip the intermediate "-1" blend target.
        {
            AnimExprBlendT* Blend=dynamic_cast<AnimExprBlendT*>(&*BlendFrom);

            if (Blend && Blend->GetFrac()==0.0f)
                BlendFrom=Blend->GetA();
        }

        m_AnimState.LastStdAE=m_Model->GetAnimExprPool().GetStandard(m_Selection[ANIM].Size()==0 ? -1 : m_Selection[ANIM][0], 0.0f);
        m_AnimState.Pose.SetAnimExpr(m_Model->GetAnimExprPool().GetBlend(BlendFrom, m_AnimState.LastStdAE, 3.0f));

        if (m_Selection[ANIM].Size()==0)
        {
            m_SequenceBB=m_AnimState.Pose.GetBB();
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
        m_Submodels.PushBack(new SubmodelT(LoadModel(FileName)));
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

    if (NextAnimSequNr < m_Model->GetAnims().Size())
        NextSel.PushBack(NextAnimSequNr);

    return NextSel;
}


ArrayT<unsigned int> ModelEditor::ModelDocumentT::GetSelection_PrevAnimSequ() const
{
    ArrayT<unsigned int> NextSel;
    const unsigned int   NextAnimSequNr=m_Selection[ANIM].Size()==0 ? m_Model->GetAnims().Size()-1 : m_Selection[ANIM][0]-1;

    if (NextAnimSequNr < m_Model->GetAnims().Size())
        NextSel.PushBack(NextAnimSequNr);

    return NextSel;
}


void ModelEditor::ModelDocumentT::AdvanceTime(float Time)
{
    if (Time*m_AnimState.Speed!=0.0f)
    {
        m_AnimState.LastStdAE->SetForceLoop(m_AnimState.Loop);
        m_AnimState.Pose.GetAnimExpr()->AdvanceTime(Time*m_AnimState.Speed);
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
    const std::string   fn    = FileName.ToStdString();
    const int           Flags = ModelLoaderT::REMOVE_DEGEN_TRIANGLES | ModelLoaderT::REMOVE_UNUSED_VERTICES | ModelLoaderT::REMOVE_UNUSED_WEIGHTS;
    ModelUserCallbacksT UserCbs;

    // TODO: This duplicates the code in Model_proxy.cpp and should be combined elsewhere, e.g. into class ModelLoaderT.
    //       Better yet: Use the type system with the loaders, and be able to iterate over them.
         if (cf::String::EndsWith(fn, "3ds"    )) { LoaderFbxT   Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "ase"    )) { LoaderAseT   Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "cmdl"   )) { LoaderCafuT  Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "dae"    )) { LoaderFbxT   Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "dummy"  )) { LoaderDummyT Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "dxf"    )) { LoaderFbxT   Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "fbx"    )) { LoaderFbxT   Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "dlod"   )) { LoaderDlodT  Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "lwo"    )) { LoaderLwoT   Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "mdl"    )) { LoaderMdlT   Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "md5"    )) { LoaderMd5T   Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "md5mesh")) { LoaderMd5T   Loader(fn, Flags);          return new CafuModelT(Loader); }
    else if (cf::String::EndsWith(fn, "obj"    )) { LoaderFbxT   Loader(fn, UserCbs, Flags); return new CafuModelT(Loader); }

    throw ModelLoaderT::LoadErrorT(
        "No loader is available for model files of this type.\n"
        "Export or convert your model to one of the supported file formats, "
        "or contact the support forums for additional help.");

    // We never get here.
    return NULL;
}
