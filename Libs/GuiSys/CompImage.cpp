/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompImage.hpp"
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

#if defined(_WIN32) && defined(_MSC_VER)
    // Turn off warning C4355: 'this' : used in base member initializer list.
    #pragma warning(disable:4355)
#endif

using namespace cf::GuiSys;


/************************************/
/*** ComponentImageT::VarMatNameT ***/
/************************************/

ComponentImageT::VarMatNameT::VarMatNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentImageT& CompImg)
    : TypeSys::VarT<std::string>(Name, Value, Flags),
      m_CompImg(CompImg)
{
}


// The compiler-written copy constructor would copy m_CompImg from Var.m_CompImg,
// but we must obviously use the reference to the proper parent instance instead.
ComponentImageT::VarMatNameT::VarMatNameT(const VarMatNameT& Var, ComponentImageT& CompImg)
    : TypeSys::VarT<std::string>(Var),
      m_CompImg(CompImg)
{
}


void ComponentImageT::VarMatNameT::Set(const std::string& v)
{
    // Make sure that m_CompImg actually refers to the ComponentImageT instance that contains us!
    assert(this == &m_CompImg.m_MatName);

    TypeSys::VarT<std::string>::Set(v);

    MatSys::Renderer->FreeMaterial(m_CompImg.m_MatInst);
    m_CompImg.m_MatInst = NULL;

    if (m_CompImg.GetWindow() && !v.empty())
    {
        m_CompImg.m_MatInst = MatSys::Renderer->RegisterMaterial(
            m_CompImg.GetWindow()->GetGui().GetMaterialManager().GetMaterial(v));
    }
}


/***********************/
/*** ComponentImageT ***/
/***********************/

namespace
{
    const char* FlagsIsColor[] = { "IsColor", NULL };
    const char* FlagsIsMaterial[] = { "IsMaterial", NULL };
}


const char* ComponentImageT::DocClass =
    "This component adds an image to its window.";


const cf::TypeSys::VarsDocT ComponentImageT::DocVars[] =
{
    { "Material", "The name of the image material." },
    { "Color",    "The color with which the image is tinted." },
    { "Alpha",    "The alpha component of the color." },
    { NULL, NULL }
};


ComponentImageT::ComponentImageT()
    : ComponentBaseT(),
      m_MatName("Material", "", FlagsIsMaterial, *this),
      m_MatInst(NULL),
      m_Color("Color", Vector3fT(1, 1, 1), FlagsIsColor),
      m_Alpha("Alpha", 1.0f)
{
    FillMemberVars();
}


ComponentImageT::ComponentImageT(const ComponentImageT& Comp)
    : ComponentBaseT(Comp),
      m_MatName(Comp.m_MatName, *this),
      m_MatInst(NULL),
      m_Color(Comp.m_Color),
      m_Alpha(Comp.m_Alpha)
{
    // There is no need to do anything with m_MatInst here:
    assert(GetWindow() == NULL);

    FillMemberVars();
}


void ComponentImageT::FillMemberVars()
{
    GetMemberVars().Add(&m_MatName);
    GetMemberVars().Add(&m_Color);
    GetMemberVars().Add(&m_Alpha);
}


ComponentImageT::~ComponentImageT()
{
    // Applications can work with `ComponentImageT`s before having initialized
    // the `MatSys::Renderer`, for example in `AppCaWE::WriteLuaDoxygenHeaders()`.
    if (MatSys::Renderer)
    {
        MatSys::Renderer->FreeMaterial(m_MatInst);
        m_MatInst = NULL;
    }
}


ComponentImageT* ComponentImageT::Clone() const
{
    return new ComponentImageT(*this);
}


void ComponentImageT::UpdateDependencies(WindowT* Window)
{
    const bool WindowChanged = Window != GetWindow();

    ComponentBaseT::UpdateDependencies(Window);

    // m_Transform = NULL;

    if (WindowChanged)
    {
        MatSys::Renderer->FreeMaterial(m_MatInst);
        m_MatInst = NULL;
    }

    if (!GetWindow()) return;


    // // It would be possible to break this loop as soon as we have assigned a non-NULL pointer to m_Transform.
    // // However, this is only because the Transform component is, at this time, the only sibling component that
    // // we're interested in, whereas the loop below is suitable for resolving additional dependencies, too.
    // for (unsigned int CompNr = 0; CompNr < GetWindow()->GetComponents().Size(); CompNr++)
    // {
    //     IntrusivePtrT<ComponentBaseT> Comp = GetWindow()->GetComponents()[CompNr];
    //
    //     if (m_Transform == NULL)
    //         m_Transform = dynamic_pointer_cast<ComponentTransformT>(Comp);
    // }

    if (WindowChanged && !m_MatName.Get().empty())
    {
        m_MatInst = MatSys::Renderer->RegisterMaterial(
            GetWindow()->GetGui().GetMaterialManager().GetMaterial(m_MatName.Get()));
    }
}


void ComponentImageT::Render() const
{
 // MatSys::Renderer->SetCurrentAmbientLightColor(m_Color);
    MatSys::Renderer->SetCurrentMaterial(m_MatInst ? m_MatInst : GetWindow()->GetGui().GetDefaultRM());

    static MatSys::MeshT BackMesh(MatSys::MeshT::Quads);

    BackMesh.Vertices.Overwrite();
    BackMesh.Vertices.PushBackEmpty(4);     // Just a single quad for the image rectangle.

    for (unsigned int VertexNr = 0; VertexNr < 4; VertexNr++)
    {
        for (unsigned int i = 0; i < 3; i++)
            BackMesh.Vertices[VertexNr].Color[i] = m_Color.Get()[i];

        BackMesh.Vertices[VertexNr].Color[3] = m_Alpha.Get();
    }

    const float x1 = 0.0f;
    const float y1 = 0.0f;
    const float x2 = GetWindow()->GetTransform()->GetSize().x;
    const float y2 = GetWindow()->GetTransform()->GetSize().y;

    BackMesh.Vertices[0].SetOrigin(x1, y1); BackMesh.Vertices[0].SetTextureCoord(0.0f, 0.0f);
    BackMesh.Vertices[1].SetOrigin(x2, y1); BackMesh.Vertices[1].SetTextureCoord(1.0f, 0.0f);
    BackMesh.Vertices[2].SetOrigin(x2, y2); BackMesh.Vertices[2].SetTextureCoord(1.0f, 1.0f);
    BackMesh.Vertices[3].SetOrigin(x1, y2); BackMesh.Vertices[3].SetTextureCoord(0.0f, 1.0f);

    MatSys::Renderer->RenderMesh(BackMesh);
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentImageT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentImageT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentImageT> >(1);

    lua_pushfstring(LuaState, "image component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentImageT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentImageT();
}

const luaL_Reg ComponentImageT::MethodsList[] =
{
    { "__tostring", toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentImageT::DocMethods[] =
{
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentImageT::TypeInfo(GetComponentTIM(), "GuiSys::ComponentImageT", "GuiSys::ComponentBaseT", ComponentImageT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
