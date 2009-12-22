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

#include "WindowModel.hpp"
#include "GuiImpl.hpp"
#include "WindowCreateParams.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Math3D/Matrix.hpp"

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
      Model(""),
      ModelSequNr(0),
      ModelFrameNr(0.0f),
      ModelPos(0, 0, 0),
      ModelScale(1.0f),
      ModelAngles(0, 0, 0),
      CameraPos(0, -200.0f, 0)
{
    FillMemberVars();
}


ModelWindowT::ModelWindowT(const ModelWindowT& Window, bool Recursive)
    : WindowT(Window, Recursive),
      Model(Window.Model),
      ModelSequNr(Window.ModelSequNr),
      ModelFrameNr(Window.ModelFrameNr),
      ModelPos(Window.ModelPos),
      ModelScale(Window.ModelScale),
      ModelAngles(Window.ModelAngles),
      CameraPos(Window.CameraPos)
{
    FillMemberVars();
}


ModelWindowT* ModelWindowT::Clone(bool Recursive) const
{
    return new ModelWindowT(*this, Recursive);
}


ModelWindowT::~ModelWindowT()
{
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

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, MatrixT::GetRotateXMatrix(-90.0f));   // Rotate coordinate system axes to Ca3DE standard.
    MatSys::Renderer->Translate(MatSys::RendererI::WORLD_TO_VIEW, -CameraPos.x, -CameraPos.y, -CameraPos.z);

    const MatrixT ProjectionMatrix=MatrixT::GetProjPerspectiveMatrix(67.5f, 640.0f/480.0f, 10.0f, 10000.0f);
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION, ProjectionMatrix);


    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentAmbientLightColor(1.0f, 1.0f, 1.0f);
    MatSys::Renderer->SetCurrentEyePosition(CameraPos.x, CameraPos.y, CameraPos.z);     // Required in some ambient shaders.

    Model.Draw(ModelSequNr, ModelFrameNr, 0.0f, NULL);

    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );
}


bool ModelWindowT::OnClockTickEvent(float t)
{
    ModelFrameNr=Model.AdvanceFrameNr(ModelSequNr, ModelFrameNr, t, true);

    return WindowT::OnClockTickEvent(t);
}


void ModelWindowT::FillMemberVars()
{
    WindowT::FillMemberVars();

    MemberVars["modelSequNr"]=MemberVarT(ModelSequNr);
    MemberVars["modelFrameNr"]=MemberVarT(ModelFrameNr);
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
    ModelWindowT* ModelWin=(ModelWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    ModelWin->Model=ModelProxyT(luaL_checkstring(LuaState, 2));
    return 0;
}


int ModelWindowT::GetModelNrOfSequs(lua_State* LuaState)
{
    ModelWindowT* ModelWin=(ModelWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    lua_pushinteger(LuaState, ModelWin->Model.GetNrOfSequences());
    return 1;
}


int ModelWindowT::SetModelSequNr(lua_State* LuaState)
{
    ModelWindowT* ModelWin=(ModelWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    ModelWin->ModelSequNr=luaL_checkinteger(LuaState, 2);
    return 0;
}


int ModelWindowT::SetModelPos(lua_State* LuaState)
{
    ModelWindowT* ModelWin=(ModelWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    ModelWin->ModelPos.x=float(lua_tonumber(LuaState, 2));
    ModelWin->ModelPos.y=float(lua_tonumber(LuaState, 3));
    ModelWin->ModelPos.z=float(lua_tonumber(LuaState, 4));
    return 0;
}


int ModelWindowT::SetModelScale(lua_State* LuaState)
{
    ModelWindowT* ModelWin=(ModelWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    ModelWin->ModelScale=float(lua_tonumber(LuaState, 2));
    return 0;
}


int ModelWindowT::SetModelAngles(lua_State* LuaState)
{
    ModelWindowT* ModelWin=(ModelWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    ModelWin->ModelAngles.x=float(lua_tonumber(LuaState, 2));
    ModelWin->ModelAngles.y=float(lua_tonumber(LuaState, 3));
    ModelWin->ModelAngles.z=float(lua_tonumber(LuaState, 4));
    return 0;
}


int ModelWindowT::SetCameraPos(lua_State* LuaState)
{
    ModelWindowT* ModelWin=(ModelWindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 1, TypeInfo);

    ModelWin->CameraPos.x=float(lua_tonumber(LuaState, 2));
    ModelWin->CameraPos.y=float(lua_tonumber(LuaState, 3));
    ModelWin->CameraPos.z=float(lua_tonumber(LuaState, 4));
    return 0;
}
