/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompSelection.hpp"

#include "GuiSys/AllComponents.hpp"
#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace GuiEditor;


void* ComponentSelectionT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentSelectionT();
}

const luaL_Reg ComponentSelectionT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentSelectionT::TypeInfo(cf::GuiSys::GetComponentTIM(), "GuiEditor::ComponentSelectionT", "GuiSys::ComponentBaseT", ComponentSelectionT::CreateInstance, MethodsList);


ComponentSelectionT::ComponentSelectionT()
    : ComponentBaseT(),
      m_IsSelected(false)
{
}


ComponentSelectionT::ComponentSelectionT(const ComponentSelectionT& Comp)
    : ComponentBaseT(Comp),
      m_IsSelected(false)
{
}


ComponentSelectionT* ComponentSelectionT::Clone() const
{
    return new ComponentSelectionT(*this);
}


void ComponentSelectionT::Render() const
{
    if (!m_IsSelected) return;

    const float b  = 2.0f;
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
                BorderMesh.Vertices[VertexNr].Color[i] = (i == 0) ? 1.0f : 0.0f;

            BorderMesh.Vertices[VertexNr].Color[3] = 1.0f;
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


int ComponentSelectionT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentBaseT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentBaseT> >(1);

    lua_pushfstring(LuaState, "selection component");
    return 1;
}
