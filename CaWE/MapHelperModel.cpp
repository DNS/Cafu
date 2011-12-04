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

#include "MapHelperModel.hpp"
#include "Camera.hpp"
#include "ChildFrameViewWin3D.hpp"
#include "EntityClass.hpp"
#include "GameConfig.hpp"
#include "LuaAux.hpp"
#include "MapEntity.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "Options.hpp"

#include "MaterialSystem/Mesh.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Models/Model_cmdl.hpp"
#include "EditorMaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "TypeSys.hpp"


/*** Begin of TypeSys related definitions for this class. ***/

void* MapHelperModelT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return NULL;
}

const cf::TypeSys::TypeInfoT MapHelperModelT::TypeInfo(GetMapElemTIM(), "MapHelperModelT", "MapHelperT", MapHelperModelT::CreateInstance, NULL);

/*** End of TypeSys related definitions for this class. ***/


MapHelperModelT::MapHelperModelT(const MapEntityT* ParentEntity, const HelperInfoT* HelperInfo)
    : MapHelperT(ParentEntity),
      m_HelperInfo(HelperInfo),
      m_Model(NULL),
      m_ModelFrameNr(0.0f),
      m_GuiNames(),
      m_Guis(),
      m_Timer()
{
}


MapHelperModelT::MapHelperModelT(const MapHelperModelT& Model)
    : MapHelperT(Model),
      m_HelperInfo(Model.m_HelperInfo),
      m_Model(Model.m_Model),
      m_ModelFrameNr(0.0f),
      m_GuiNames(),
      m_Guis(),
      m_Timer(Model.m_Timer)
{
}


MapHelperModelT::~MapHelperModelT()
{
    ClearGuis();
}


MapHelperModelT* MapHelperModelT::Clone() const
{
    return new MapHelperModelT(*this);
}


void MapHelperModelT::Assign(const MapElementT* Elem)
{
    if (Elem==this) return;

    MapHelperT::Assign(Elem);

    const MapHelperModelT* Model=dynamic_cast<const MapHelperModelT*>(Elem);
    wxASSERT(Model!=NULL);
    if (Model==NULL) return;

    m_ParentEntity=Model->m_ParentEntity;
    m_HelperInfo  =Model->m_HelperInfo;
    m_Model       =Model->m_Model;
    m_ModelFrameNr=Model->m_ModelFrameNr;
    m_Timer       =Model->m_Timer;

    ClearGuis();
}


BoundingBox3fT MapHelperModelT::GetBB() const
{
    const Vector3fT Origin=m_ParentEntity->GetOrigin();

    UpdateModelCache();

    // TODO: Cache!
    const cf::math::AnglesfT Angles=m_ParentEntity->GetAngles();

    // The 3D bounds are the bounds of the oriented model's first sequence, so that frustum culling works properly in the 3D view.
    Vector3fT VerticesBB[8];
    m_Model->GetSharedPose(GetSequenceNr(), 0.0f)->GetBB().GetCornerVertices(VerticesBB);

    // Rotate all eight vertices.
    for (unsigned long VertexNr=0; VertexNr<8; VertexNr++)
        VerticesBB[VertexNr]=VerticesBB[VertexNr].GetRotX(Angles[ROLL]).GetRotY(-Angles[PITCH]).GetRotZ(Angles[YAW]);

    // Build a new BB of the rotated BB.
    BoundingBox3fT RotBB(VerticesBB[0]);

    for (unsigned long VertexNr=1; VertexNr<8; VertexNr++)
        RotBB.Insert(VerticesBB[VertexNr]);

    RotBB.Min+=Origin;
    RotBB.Max+=Origin;

    return RotBB;
}


void MapHelperModelT::Render2D(Renderer2DT& Renderer) const
{
    // Render nothing in 2D, as the parent entity already renders its bounding box,
    // center cross, orientation (angles), etc. by itself.
    // UpdateModelCache();  // No need to update the model cache when we don't render anything anyway.
}


void MapHelperModelT::Render3D(Renderer3DT& Renderer) const
{
    const Vector3fT Origin    =m_ParentEntity->GetOrigin();
    const int       SequenceNr=GetSequenceNr();

    UpdateModelCache();

    const Vector3fT ViewPoint=Renderer.GetViewWin3D().GetCamera().Pos;
    const float     ModelDist=length(Origin-ViewPoint);
    AnimPoseT*      Pose     =m_Model->GetSharedPose(SequenceNr, m_ModelFrameNr);

    if (Options.view3d.AnimateModels)
    {
        Pose->Advance(float(m_Timer.GetSecondsSinceLastCall()));

        m_ModelFrameNr=Pose->GetFrameNr();
    }

    if (ModelDist < float(Options.view3d.ModelDistance))
    {
        const cf::math::AnglesfT Angles=m_ParentEntity->GetAngles();
        const float              CAFU_ENG_SCALE=25.4f;

        MatSys::Renderer->SetCurrentAmbientLightColor(1.0f, 1.0f, 1.0f);
        MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, Origin[0], Origin[1], Origin[2]);
        MatSys::Renderer->RotateZ  (MatSys::RendererI::MODEL_TO_WORLD,  Angles[YAW  ]);
        MatSys::Renderer->RotateY  (MatSys::RendererI::MODEL_TO_WORLD, -Angles[PITCH]);
        MatSys::Renderer->RotateX  (MatSys::RendererI::MODEL_TO_WORLD,  Angles[ROLL ]);

        Pose->Draw(-1 /*default skin*/, CAFU_ENG_SCALE*ModelDist);

        const MatrixT ModelToWorld=MatSys::Renderer->GetMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        for (unsigned long GFNr=0; GFNr<m_Model->GetGuiFixtures().Size(); GFNr++)
        {
            Vector3fT GuiOrigin;
            Vector3fT GuiAxisX;
            Vector3fT GuiAxisY;

            if (Pose->GetGuiPlane(GFNr, GuiOrigin, GuiAxisX, GuiAxisY))
            {
                // It's pretty easy to derive this matrix geometrically, see my TechArchive note from 2006-08-22.
                MatrixT M(GuiAxisX.x/640.0f, GuiAxisY.x/480.0f, 0.0f, GuiOrigin.x,
                          GuiAxisX.y/640.0f, GuiAxisY.y/480.0f, 0.0f, GuiOrigin.y,
                          GuiAxisX.z/640.0f, GuiAxisY.z/480.0f, 0.0f, GuiOrigin.z,
                                       0.0f,              0.0f, 0.0f,        1.0f);

                MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, ModelToWorld*M);

                m_Guis[GFNr]->Render(true /*zLayerCoating*/);
            }
        }

        MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        if (m_ParentEntity->IsSelected()) Renderer.RenderBox(GetBB(), Options.colors.Selection, false /* Solid? */);
    }
    else
    {
        // Did not render the real model (the distance was too great), thus render a replacement bounding-box.
        Renderer.RenderBox(GetBB(),
            m_ParentEntity->IsSelected() ? Options.colors.Selection : m_ParentEntity->GetColor(Options.view2d.UseGroupColors), true /* Solid? */);
    }
}


void MapHelperModelT::ClearGuis() const
{
    for (unsigned long GuiNr=0; GuiNr<m_Guis.Size(); GuiNr++)
    {
        delete m_Guis[GuiNr];
        m_Guis[GuiNr]=NULL;
    }

    m_GuiNames.Overwrite();
    m_Guis.Overwrite();
}


void MapHelperModelT::UpdateModelCache() const
{
    // This const_cast<> is a horrible hack -- we really should have a better means of getting a non-const GameConfigT here.
    GameConfigT& GameConfig=const_cast<GameConfigT&>(m_ParentEntity->GetClass()->GetGameConfig());

    wxString ModelName="";
    wxString ErrorMsg ="";

    if (m_HelperInfo->Parameters.Size() > 0)
    {
        ModelName=m_HelperInfo->Parameters[0];
    }
    else
    {
        // If we weren't passed a model name as an argument, get it from our parent entity's "model" property.
        // Calling FindProperty() each render frame is not particularly efficient...
        const EntPropertyT* ModelProp=m_ParentEntity->FindProperty("model");

        if (ModelProp) ModelName=ModelProp->Value;
    }

    const CafuModelT* Model=GameConfig.GetModel(ModelName, &ErrorMsg);

    if (m_Model!=Model)
    {
        const std::string PrevFileName=m_Model ? m_Model->GetFileName() : "<none>";

        wxLogDebug("MapHelperModelT::UpdateModelCache(): Updating model from %s to %s. %s", PrevFileName, Model->GetFileName(), ErrorMsg);

        m_Model       =Model;
        m_ModelFrameNr=0.0f;

        ClearGuis();
    }


    while (m_GuiNames.Size() < m_Model->GetGuiFixtures().Size()) m_GuiNames.PushBack("");
    while (m_Guis.Size()     < m_Model->GetGuiFixtures().Size()) m_Guis.PushBack(NULL);

    for (unsigned long GFNr=0; GFNr<m_Model->GetGuiFixtures().Size(); GFNr++)
    {
        const EntPropertyT* GuiProp=m_ParentEntity->FindProperty("gui");

        if (!GuiProp || GFNr>0)
            GuiProp=m_ParentEntity->FindProperty(wxString::Format("gui%lu", GFNr+1));

        const wxString NewGuiName=GuiProp ? GuiProp->Value : "";

        if (!m_Guis[GFNr] || NewGuiName!=m_GuiNames[GFNr])
        {
            m_GuiNames[GFNr]=NewGuiName;
            delete m_Guis[GFNr];
            m_Guis[GFNr]=NULL;

            try
            {
                if (m_GuiNames[GFNr]=="")
                {
                    m_Guis[GFNr]=new cf::GuiSys::GuiImplT(GameConfig.GetGuiResources(),
                        "Win1=gui:new('WindowT'); gui:SetRootWindow(Win1); gui:activate(true); "
                        "gui:setInteractive(true); gui:showMouse(false); Win1:set('rect', 0, 0, 640, 480); "
                        "Win1:set('backColor', 150/255, 170/255, 204/255, 0.8); "
                        "Win1:set('textAlignHor', 2); Win1:set('textAlignVer', 2); "
                        "Win1:set('textColor', 15/255, 49/255, 106/255); "
                        "Win1:set('text', 'This is a\\nfull-scale sample GUI.\\n\\n"
                        "Set the \\\""+ std::string(GuiProp->Key) +"\\\" entity property\\nto assign the true GUI.');", true);
                }
                else
                {
                    m_Guis[GFNr]=new cf::GuiSys::GuiImplT(GameConfig.GetGuiResources(), std::string(GameConfig.ModDir + "/" + m_GuiNames[GFNr]));
                }
            }
            catch (const cf::GuiSys::GuiImplT::InitErrorT& IE)
            {
                // This one must not throw again...
                m_Guis[GFNr]=new cf::GuiSys::GuiImplT(GameConfig.GetGuiResources(),
                    "Win1=gui:new('WindowT'); gui:SetRootWindow(Win1); gui:activate(true); "
                    "gui:setInteractive(true); gui:showMouse(false); Win1:set('rect', 0, 0, 640, 480); "
                    "Win1:set('backColor', 150/255, 170/255, 204/255, 0.8); "
                    "Win1:set('textAlignHor', 2); Win1:set('textAlignVer', 2); "
                    "Win1:set('textColor', 15/255, 49/255, 106/255); "
                    "Win1:set('textScale', 0.6); "
                    "Win1:set('text', [=====[Could not load GUI\n" +
                    std::string(GameConfig.ModDir + "/" + m_GuiNames[GFNr]) + "\n\n" + IE.what() + "]=====]);", true);
            }
        }
    }
}


int MapHelperModelT::GetSequenceNr() const
{
    // Calling FindProperty() each render frame is not particularly efficient...
    const EntPropertyT* Prop=m_ParentEntity->FindProperty("sequence");

    return Prop ? wxAtoi(Prop->Value) : 0;
}
