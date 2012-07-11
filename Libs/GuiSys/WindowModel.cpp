/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#include "WindowModel.hpp"
#include "GuiImpl.hpp"
#include "WindowCreateParams.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"
#include "Models/AnimPose.hpp"
#include "Models/Model_cmdl.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


using namespace cf::GuiSys;


/*** Begin of TypeSys related definitions for this class. ***/

void* ModelWindowT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ModelWindowT(*static_cast<const cf::GuiSys::WindowCreateParamsT*>(&Params));
}

const luaL_reg ModelWindowT::MethodsList[]=
{
    { "SetModel",        ModelWindowT::SetModel },
    { "GetModelNrOfSqs", ModelWindowT::GetModelNrOfSequs },
    { "SetModelSequNr",  ModelWindowT::SetModelSequNr },
    { "SetModelPos",     ModelWindowT::SetModelPos },
    { "SetModelScale",   ModelWindowT::SetModelScale },
    { "SetModelAngles",  ModelWindowT::SetModelAngles },
    { "SetCameraPos",    ModelWindowT::SetCameraPos },
    { "__gc",            WindowT::Destruct },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ModelWindowT::TypeInfo(GetWindowTIM(), "ModelWindowT", "WindowT", ModelWindowT::CreateInstance, MethodsList);

/*** End of TypeSys related definitions for this class. ***/


ModelWindowT::ModelWindowT(const cf::GuiSys::WindowCreateParamsT& Params)
    : WindowT(Params),
      m_Model(NULL),
      m_Pose(NULL),
      m_LastStdAE(),
      ModelPos(0, 0, 0),
      ModelScale(1.0f),
      ModelAngles(0, 0, 0),
      CameraPos(0, -200.0f, 0)
{
    std::string ErrorMsg;

    SetModel("dummy", ErrorMsg);
    FillMemberVars();
}


ModelWindowT::ModelWindowT(const ModelWindowT& Window, bool Recursive)
    : WindowT(Window, Recursive),
      m_Model(NULL),
      m_Pose(NULL),
      m_LastStdAE(),
      ModelPos(Window.ModelPos),
      ModelScale(Window.ModelScale),
      ModelAngles(Window.ModelAngles),
      CameraPos(Window.CameraPos)
{
    std::string ErrorMsg;

    SetModel(Window.m_Model->GetFileName(), ErrorMsg);
    FillMemberVars();
}


ModelWindowT* ModelWindowT::Clone(bool Recursive) const
{
    return new ModelWindowT(*this, Recursive);
}


ModelWindowT::~ModelWindowT()
{
    delete m_Pose;
}


void ModelWindowT::SetModel(const std::string& FileName, std::string& ErrorMsg)
{
    const CafuModelT* PrevModel=m_Model;

    m_Model=m_Gui.GetGuiResources().GetModel(FileName, ErrorMsg);

    if (m_Pose==NULL || PrevModel!=m_Model)
    {
        m_LastStdAE=m_Model->GetAnimExprPool().GetStandard(0, 0.0f);
        m_LastStdAE->SetForceLoop(true);

        delete m_Pose;
        m_Pose=new AnimPoseT(*m_Model, m_LastStdAE);
    }
}


int ModelWindowT::GetModelSequNr() const
{
    return m_LastStdAE->GetSequNr();
}


void ModelWindowT::Render() const
{
    WindowT::Render();      // Uses (or should use) only materials with "ambientMask d" set.

    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );


    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, ModelPos.x, ModelPos.y, ModelPos.z);
    MatSys::Renderer->Scale    (MatSys::RendererI::MODEL_TO_WORLD, ModelScale);
    if (ModelAngles.x!=0) MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, ModelAngles.x);
    if (ModelAngles.y!=0) MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, ModelAngles.y);
    if (ModelAngles.z!=0) MatSys::Renderer->RotateZ(MatSys::RendererI::MODEL_TO_WORLD, ModelAngles.z);

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, MatrixT::GetRotateXMatrix(-90.0f));   // Rotate coordinate system axes to Cafu standard.
    MatSys::Renderer->Translate(MatSys::RendererI::WORLD_TO_VIEW, -CameraPos.x, -CameraPos.y, -CameraPos.z);

    const MatrixT ProjectionMatrix=MatrixT::GetProjPerspectiveMatrix(67.5f, 640.0f/480.0f, 10.0f, 10000.0f);
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION, ProjectionMatrix);


    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentAmbientLightColor(1.0f, 1.0f, 1.0f);
    MatSys::Renderer->SetCurrentEyePosition(CameraPos.x, CameraPos.y, CameraPos.z);     // Required in some ambient shaders.

    m_Pose->Draw(-1 /*default skin*/, 0.0f);

    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );
}


bool ModelWindowT::OnClockTickEvent(float t)
{
    m_Pose->GetAnimExpr()->AdvanceTime(t);

    return WindowT::OnClockTickEvent(t);
}


void ModelWindowT::FillMemberVars()
{
    WindowT::FillMemberVars();

    MemberVars["modelPos.x"]=MemberVarT(ModelPos.x);
    MemberVars["modelPos.y"]=MemberVarT(ModelPos.y);
    MemberVars["modelPos.z"]=MemberVarT(ModelPos.z);
    MemberVars["modelScale"]=MemberVarT(ModelScale);
    MemberVars["modelAngles.x"]=MemberVarT(ModelAngles.x);
    MemberVars["modelAngles.y"]=MemberVarT(ModelAngles.y);
    MemberVars["modelAngles.z"]=MemberVarT(ModelAngles.z);
    MemberVars["cameraPos.x"]=MemberVarT(CameraPos.x);
    MemberVars["cameraPos.y"]=MemberVarT(CameraPos.y);
    MemberVars["cameraPos.z"]=MemberVarT(CameraPos.z);
}


int ModelWindowT::SetModel(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    std::string   ErrorMsg;
    ModelWindowT* ModelWin=(ModelWindowT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ModelWin->SetModel(luaL_checkstring(LuaState, 2), ErrorMsg);

    if (ErrorMsg!="") return luaL_error(LuaState, "%s", ErrorMsg.c_str());
    return 0;
}


int ModelWindowT::GetModelNrOfSequs(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ModelWindowT* ModelWin=(ModelWindowT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    lua_pushinteger(LuaState, ModelWin->m_Model->GetAnims().Size());
    return 1;
}


int ModelWindowT::SetModelSequNr(lua_State* LuaState)
{
    ScriptBinderT     Binder(LuaState);
    ModelWindowT*     ModelWin=(ModelWindowT*)Binder.GetCheckedObjectParam(1, TypeInfo);
    const CafuModelT* Model   =ModelWin->m_Model;
    AnimPoseT*        Pose    =ModelWin->m_Pose;

    IntrusivePtrT<AnimExpressionT> BlendFrom=Pose->GetAnimExpr();

    ModelWin->m_LastStdAE=Model->GetAnimExprPool().GetStandard(luaL_checkinteger(LuaState, 2), 0.0f);
    ModelWin->m_LastStdAE->SetForceLoop(true);

    Pose->SetAnimExpr(Model->GetAnimExprPool().GetBlend(BlendFrom, ModelWin->m_LastStdAE, 3.0f));
    return 0;
}


int ModelWindowT::SetModelPos(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ModelWindowT* ModelWin=(ModelWindowT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ModelWin->ModelPos.x=float(lua_tonumber(LuaState, 2));
    ModelWin->ModelPos.y=float(lua_tonumber(LuaState, 3));
    ModelWin->ModelPos.z=float(lua_tonumber(LuaState, 4));
    return 0;
}


int ModelWindowT::SetModelScale(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ModelWindowT* ModelWin=(ModelWindowT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ModelWin->ModelScale=float(lua_tonumber(LuaState, 2));
    return 0;
}


int ModelWindowT::SetModelAngles(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ModelWindowT* ModelWin=(ModelWindowT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ModelWin->ModelAngles.x=float(lua_tonumber(LuaState, 2));
    ModelWin->ModelAngles.y=float(lua_tonumber(LuaState, 3));
    ModelWin->ModelAngles.z=float(lua_tonumber(LuaState, 4));
    return 0;
}


int ModelWindowT::SetCameraPos(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    ModelWindowT* ModelWin=(ModelWindowT*)Binder.GetCheckedObjectParam(1, TypeInfo);

    ModelWin->CameraPos.x=float(lua_tonumber(LuaState, 2));
    ModelWin->CameraPos.y=float(lua_tonumber(LuaState, 3));
    ModelWin->CameraPos.z=float(lua_tonumber(LuaState, 4));
    return 0;
}
