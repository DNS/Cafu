/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompBorder.hpp"
#include "AllComponents.hpp"
#include "GuiImpl.hpp"
#include "Window.hpp"

#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GuiSys;


namespace
{
    const char* FlagsIsColor[] = { "IsColor", NULL };
}


const char* ComponentBorderT::DocClass =
    "This components adds a border to its window.";


const cf::TypeSys::VarsDocT ComponentBorderT::DocVars[] =
{
    { "Width", "The width of the border." },
    { "Color", "The border color." },
    { "Alpha", "The alpha component of the color." },
    { NULL, NULL }
};


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
    const float x2 = GetWindow()->GetTransform()->GetSize().x;
    const float y2 = GetWindow()->GetTransform()->GetSize().y;

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


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentBorderT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "border component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentBorderT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentBorderT();
}

const luaL_Reg ComponentBorderT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentBorderT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentBorderT::TypeInfo(GetComponentTIM(), "GuiSys::ComponentBorderT", "GuiSys::ComponentBaseT", ComponentBorderT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
