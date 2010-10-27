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
#include "Models/Model_dummy.hpp"
#include "EditorMaterialManager.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "TypeSys.hpp"

#include "wx/wx.h"
#include "wx/sstream.h"
#include "wx/txtstrm.h"


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
      m_ModelProxy(),
      m_ModelFrameNr(0.0f),
      m_Timer()
{
}


MapHelperModelT::MapHelperModelT(const MapHelperModelT& Model)
    : MapHelperT(Model),
      m_HelperInfo(Model.m_HelperInfo),
      m_ModelProxy(Model.m_ModelProxy),
      m_ModelFrameNr(0.0f),
      m_Timer(Model.m_Timer)
{
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
    m_ModelProxy  =Model->m_ModelProxy;
    m_ModelFrameNr=Model->m_ModelFrameNr;
    m_Timer       =Model->m_Timer;
}


BoundingBox3fT MapHelperModelT::GetBB() const
{
    const Vector3fT Origin=m_ParentEntity->GetOrigin();

    UpdateModelCache();

    // TODO: Cache!
    if (dynamic_cast<const ModelDummyT*>(m_ModelProxy.GetRealModel())==NULL)
    {
        const cf::math::AnglesfT Angles=m_ParentEntity->GetAngles();

        // The 3D bounds are the bounds of the oriented model's first sequence, so that frustum culling works properly in the 3D view.
        const float* BB=m_ModelProxy.GetSequenceBB(GetSequenceNr(), 0.0f);

        // Construct all eight vertices of this BB.
        Vector3fT VerticesBB[8]=
        {
            Vector3fT(BB[0], BB[1], BB[2]), Vector3fT(BB[0], BB[1], BB[5]),
            Vector3fT(BB[0], BB[4], BB[2]), Vector3fT(BB[0], BB[4], BB[5]),
            Vector3fT(BB[3], BB[1], BB[2]), Vector3fT(BB[3], BB[1], BB[5]),
            Vector3fT(BB[3], BB[4], BB[2]), Vector3fT(BB[3], BB[4], BB[5])
        };

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

    return BoundingBox3fT(Origin-Vector3fT(10, 10, 10), Origin+Vector3fT(10, 10, 10));
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

    if (dynamic_cast<const ModelDummyT*>(m_ModelProxy.GetRealModel())==NULL)
    {
        const Vector3fT ViewPoint=Renderer.GetViewWin3D().GetCamera().Pos;
        const float     ModelDist=length(Origin-ViewPoint);

        if (Options.view3d.AnimateModels)
            m_ModelFrameNr=m_ModelProxy.AdvanceFrameNr(SequenceNr, m_ModelFrameNr, float(m_Timer.GetSecondsSinceLastCall()));

        if (ModelDist<float(Options.view3d.ModelDistance))
        {
            const cf::math::AnglesfT Angles=m_ParentEntity->GetAngles();
            const float              CAFU_ENG_SCALE=25.4f;

            MatSys::Renderer->SetCurrentAmbientLightColor(1.0f, 1.0f, 1.0f);
            MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);

            MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, Origin[0], Origin[1], Origin[2]);
            MatSys::Renderer->RotateZ  (MatSys::RendererI::MODEL_TO_WORLD,  Angles[YAW  ]);
            MatSys::Renderer->RotateY  (MatSys::RendererI::MODEL_TO_WORLD, -Angles[PITCH]);
            MatSys::Renderer->RotateX  (MatSys::RendererI::MODEL_TO_WORLD,  Angles[ROLL ]);

            m_ModelProxy.Draw(SequenceNr, m_ModelFrameNr, CAFU_ENG_SCALE*ModelDist, NULL);

            MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);

            if (m_ParentEntity->IsSelected()) Renderer.RenderBox(GetBB(), Options.colors.Selection, false /* Solid? */);
            return;
        }
    }

    // Did not render the real model (either because we only have a dummy model,
    // or the distance was too great), thus render a replacement bounding-box.
    Renderer.RenderBox(GetBB(),
        m_ParentEntity->IsSelected() ? Options.colors.Selection : m_ParentEntity->GetColor(Options.view2d.UseGroupColors), true /* Solid? */);
}


void MapHelperModelT::UpdateModelCache() const
{
    const wxString* ModelName=m_HelperInfo->Parameters.Size()>0 ? &m_HelperInfo->Parameters[0] : NULL;

    // If we weren't passed a model name as an argument, get it from our parent entity's "model" property.
    if (ModelName==NULL)
    {
        // Calling FindProperty() each render frame is not particularly efficient...
        const EntPropertyT* ModelProp=m_ParentEntity->FindProperty("model");

        if (ModelProp) ModelName=&ModelProp->Value;
    }

    // If the helper info has no argument and the parent has no "model" property, we shouldn't have gotten here.
    if (ModelName==NULL) return;

    const wxString FullName=m_ParentEntity->GetClass()->GetGameConfig().ModDir+"/"+(*ModelName);

    if (m_ModelProxy.GetFileName()==FullName) return;

    wxLogDebug("MapHelperModelT::UpdateModelCache(): Updating model from %s to %s.", m_ModelProxy.GetFileName(), FullName);

    m_ModelProxy=ModelProxyT(std::string(FullName));
    m_ModelFrameNr=0.0f;
}


int MapHelperModelT::GetSequenceNr() const
{
    // Calling FindProperty() each render frame is not particularly efficient...
    const EntPropertyT* Prop=m_ParentEntity->FindProperty("sequence");

    return Prop ? wxAtoi(Prop->Value) : 0;
}
