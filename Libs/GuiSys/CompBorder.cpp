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

#include "CompBorder.hpp"
#include "AllComponents.hpp"
#include "GuiImpl.hpp"
#include "Window.hpp"
#include "UniScriptState.hpp"

#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


void* ComponentBorderT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentBorderT();
}

const luaL_reg ComponentBorderT::MethodsList[] =
{
    { "__tostring", ComponentBorderT::toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentBorderT::TypeInfo(GetComponentTIM(), "ComponentBorderT", "ComponentBaseT", ComponentBorderT::CreateInstance, MethodsList);


namespace
{
    const char* FlagsIsColor[] = { "IsColor", NULL };
}


ComponentBorderT::ComponentBorderT()
    : ComponentBaseT(),
      m_Width("Width", 0.0f),
      m_Color("Color", Vector3fT(1, 1, 1), FlagsIsColor),
      m_Alpha("Alpha", 1.0f)
{
    FillMemberVars();
}


ComponentBorderT::ComponentBorderT(const ComponentBorderT& Comp)
    : ComponentBaseT(Comp),
      m_Width(Comp.m_Width),
      m_Color(Comp.m_Color),
      m_Alpha(Comp.m_Alpha)
{
    FillMemberVars();
}


void ComponentBorderT::FillMemberVars()
{
    GetMemberVars().Add(&m_Width);
    GetMemberVars().Add(&m_Color);
    GetMemberVars().Add(&m_Alpha);
}


ComponentBorderT* ComponentBorderT::Clone() const
{
    return new ComponentBorderT(*this);
}


void ComponentBorderT::Render() const
{
    const float b  = m_Width.Get();
    const float x1 = 0.0f;
    const float y1 = 0.0f;
    const float x2 = GetWindow()->Rect[2];
    const float y2 = GetWindow()->Rect[3];

    if (b > 0.0f)
    {
     // MatSys::Renderer->SetCurrentAmbientLightColor(BorderColor);
        MatSys::Renderer->SetCurrentMaterial(GetWindow()->GetGui().GetDefaultRM() /*BorderMaterial*/);

        static MatSys::MeshT BorderMesh(MatSys::MeshT::Quads);

        BorderMesh.Vertices.Overwrite();
        BorderMesh.Vertices.PushBackEmpty(4*4);     // One rectangle for each side of the background.

        for (unsigned int VertexNr = 0; VertexNr < BorderMesh.Vertices.Size(); VertexNr++)
        {
            for (unsigned int i = 0; i < 3; i++)
                BorderMesh.Vertices[VertexNr].Color[i] = m_Color.Get()[i];

            BorderMesh.Vertices[VertexNr].Color[3] = m_Alpha.Get();
        }

        // Left border rectangle.
        BorderMesh.Vertices[ 0].SetOrigin(x1,   y1); BorderMesh.Vertices[ 0].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 1].SetOrigin(x1+b, y1); BorderMesh.Vertices[ 1].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[ 2].SetOrigin(x1+b, y2); BorderMesh.Vertices[ 2].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[ 3].SetOrigin(x1,   y2); BorderMesh.Vertices[ 3].SetTextureCoord(0.0f, 1.0f);

        // Top border rectangle.
        BorderMesh.Vertices[ 4].SetOrigin(x1+b, y1  ); BorderMesh.Vertices[ 4].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 5].SetOrigin(x2-b, y1  ); BorderMesh.Vertices[ 5].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[ 6].SetOrigin(x2-b, y1+b); BorderMesh.Vertices[ 6].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[ 7].SetOrigin(x1+b, y1+b); BorderMesh.Vertices[ 7].SetTextureCoord(0.0f, 1.0f);

        // Right border rectangle.
        BorderMesh.Vertices[ 8].SetOrigin(x2-b, y1); BorderMesh.Vertices[ 8].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[ 9].SetOrigin(x2,   y1); BorderMesh.Vertices[ 9].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[10].SetOrigin(x2,   y2); BorderMesh.Vertices[10].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[11].SetOrigin(x2-b, y2); BorderMesh.Vertices[11].SetTextureCoord(0.0f, 1.0f);

        // Bottom border rectangle.
        BorderMesh.Vertices[12].SetOrigin(x1+b, y2-b); BorderMesh.Vertices[12].SetTextureCoord(0.0f, 0.0f);
        BorderMesh.Vertices[13].SetOrigin(x2-b, y2-b); BorderMesh.Vertices[13].SetTextureCoord(1.0f, 0.0f);
        BorderMesh.Vertices[14].SetOrigin(x2-b, y2  ); BorderMesh.Vertices[14].SetTextureCoord(1.0f, 1.0f);
        BorderMesh.Vertices[15].SetOrigin(x1+b, y2  ); BorderMesh.Vertices[15].SetTextureCoord(0.0f, 1.0f);

        MatSys::Renderer->RenderMesh(BorderMesh);
    }
}


int ComponentBorderT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "border component");
    return 1;
}
