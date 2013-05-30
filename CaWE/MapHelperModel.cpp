/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2013 Carsten Fuchs Software.

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
#include "CompMapEntity.hpp"
#include "EntityClass.hpp"
#include "GameConfig.hpp"
#include "LuaAux.hpp"
#include "MapDocument.hpp"
#include "MapEntRepres.hpp"
#include "Renderer2D.hpp"
#include "Renderer3D.hpp"
#include "Options.hpp"

#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "Math3D/BoundingBox.hpp"
#include "Math3D/Matrix3x3.hpp"
#include "Models/Model_cmdl.hpp"
#include "EditorMaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"


MapHelperModelT::MapHelperModelT(MapEntRepresT& Repres, const HelperInfoT* HelperInfo)
    : MapHelperT(Repres),
      m_HelperInfo(HelperInfo),
      m_Model(NULL),
      m_AnimExpr(),
      m_LastStdAE(),
      m_GuiNames(),
      m_Guis(),
      m_Timer()
{
}


MapHelperModelT::MapHelperModelT(const MapHelperModelT& Model)
    : MapHelperT(Model),
      m_HelperInfo(Model.m_HelperInfo),
      m_Model(NULL),    // Don't start with Model.m_Model, so that m_AnimExpr and m_LastStdAE are properly inited in UpdateModelCache().
      m_AnimExpr(),
      m_LastStdAE(),
      m_GuiNames(),
      m_Guis(),
      m_Timer(Model.m_Timer)
{
}


MapHelperModelT::~MapHelperModelT()
{
    ClearGuis();
}


BoundingBox3fT MapHelperModelT::GetBB() const
{
    const Vector3fT Origin = m_Repres.GetParent()->GetOrigin();

    UpdateModelCache();

    // TODO: Cache!
    const cf::math::AnglesfT Angles = m_Repres.GetParent()->GetAngles();

    // The 3D bounds are the bounds of the oriented model's first sequence, so that frustum culling works properly in the 3D view.
    Vector3fT VerticesBB[8];
    m_Model->GetSharedPose(m_AnimExpr)->GetBB().GetCornerVertices(VerticesBB);

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
    const Vector3fT Origin     = m_Repres.GetParent()->GetOrigin();
    const int       SequenceNr = GetSequenceNr();

    UpdateModelCache();

    if (SequenceNr!=m_LastStdAE->GetSequNr())
    {
        if (Options.view3d.AnimateModels)
        {
            IntrusivePtrT<AnimExpressionT> BlendFrom=m_AnimExpr;

            m_LastStdAE=m_Model->GetAnimExprPool().GetStandard(SequenceNr, 0.0f);
            m_AnimExpr =m_Model->GetAnimExprPool().GetBlend(BlendFrom, m_LastStdAE, 3.0f);
        }
        else
        {
            m_LastStdAE=m_Model->GetAnimExprPool().GetStandard(SequenceNr, 0.0f);
            m_AnimExpr =m_LastStdAE;
        }
    }

    if (Options.view3d.AnimateModels)
    {
        m_AnimExpr->AdvanceTime(float(m_Timer.GetSecondsSinceLastCall()));
    }

    const Vector3fT ViewPoint=Renderer.GetViewWin3D().GetCamera().Pos;
    const float     ModelDist=length(Origin-ViewPoint);
    AnimPoseT*      Pose     =m_Model->GetSharedPose(m_AnimExpr);

    if (ModelDist < float(Options.view3d.ModelDistance))
    {
        const cf::math::AnglesfT Angles = m_Repres.GetParent()->GetAngles();
        const float              CAFU_ENG_SCALE = 25.4f;

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

        if (m_Repres.IsSelected()) Renderer.RenderBox(GetBB(), Options.colors.Selection, false /* Solid? */);
    }
    else
    {
        // Did not render the real model (the distance was too great), thus render a replacement bounding-box.
        Renderer.RenderBox(GetBB(),
            m_Repres.IsSelected() ? Options.colors.Selection : m_Repres.GetColor(Options.view2d.UseGroupColors), true /* Solid? */);
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
    GameConfigT& GameConfig = *m_Repres.GetParent()->GetDoc().GetGameConfig();

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
        const EntPropertyT* ModelProp = m_Repres.GetParent()->FindProperty("model");

        if (ModelProp) ModelName=ModelProp->Value;
    }

    const CafuModelT* Model=GameConfig.GetModel(ModelName, &ErrorMsg);

    if (m_Model!=Model)
    {
        const std::string PrevFileName=m_Model ? m_Model->GetFileName() : "<none>";

        wxLogDebug("MapHelperModelT::UpdateModelCache(): Updating model from %s to %s. %s", PrevFileName, Model->GetFileName(), ErrorMsg);

        m_Model    =Model;
        m_LastStdAE=m_Model->GetAnimExprPool().GetStandard(GetSequenceNr(), 0.0f);
        m_AnimExpr =m_LastStdAE;

        ClearGuis();
    }


    while (m_GuiNames.Size() < m_Model->GetGuiFixtures().Size()) m_GuiNames.PushBack("");
    while (m_Guis.Size()     < m_Model->GetGuiFixtures().Size()) m_Guis.PushBack(NULL);

    for (unsigned long GFNr=0; GFNr<m_Model->GetGuiFixtures().Size(); GFNr++)
    {
        const EntPropertyT* GuiProp = m_Repres.GetParent()->FindProperty("gui");

        if (!GuiProp || GFNr>0)
            GuiProp = m_Repres.GetParent()->FindProperty(wxString::Format("gui%lu", GFNr+1));

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
                        "Set the \\\""+ std::string(GuiProp->Key) +"\\\" entity property\\nto assign the true GUI.');", cf::GuiSys::GuiImplT::InitFlag_InlineCode);
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
                    std::string(GameConfig.ModDir + "/" + m_GuiNames[GFNr]) + "\n\n" + IE.what() + "]=====]);", cf::GuiSys::GuiImplT::InitFlag_InlineCode);
            }
        }
    }
}


int MapHelperModelT::GetSequenceNr() const
{
    // Calling FindProperty() each render frame is not particularly efficient...
    const EntPropertyT* Prop = m_Repres.GetParent()->FindProperty("sequence");

    return Prop ? wxAtoi(Prop->Value) : 0;
}
