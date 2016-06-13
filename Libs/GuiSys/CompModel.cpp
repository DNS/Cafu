/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompModel.hpp"
#include "AllComponents.hpp"
#include "GuiImpl.hpp"
#include "GuiResources.hpp"
#include "Window.hpp"

#include "MaterialSystem/Renderer.hpp"
#include "Models/Model_cmdl.hpp"
#include "Network/State.hpp"
#include "UniScriptState.hpp"

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


/**************************************/
/*** ComponentModelT::VarModelNameT ***/
/**************************************/

ComponentModelT::VarModelNameT::VarModelNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentModelT& Comp)
    : TypeSys::VarT<std::string>(Name, Value, Flags),
      m_Comp(Comp)
{
}


// The compiler-written copy constructor would copy m_Comp from Var.m_Comp,
// but we must obviously use the reference to the proper parent instance instead.
ComponentModelT::VarModelNameT::VarModelNameT(const VarModelNameT& Var, ComponentModelT& Comp)
    : TypeSys::VarT<std::string>(Var),
      m_Comp(Comp)
{
}


void ComponentModelT::VarModelNameT::Serialize(cf::Network::OutStreamT& Stream) const
{
    Stream << Get();
    Stream << m_Comp.m_ModelAnimNr.Get();
    Stream << m_Comp.m_ModelSkinNr.Get();
}


void ComponentModelT::VarModelNameT::Deserialize(cf::Network::InStreamT& Stream)
{
    std::string s = "";
    int         i = 0;

    Stream >> s; Set(s);
    Stream >> i; m_Comp.m_ModelAnimNr.Set(i);
    Stream >> i; m_Comp.m_ModelSkinNr.Set(i);
}


void ComponentModelT::VarModelNameT::Set(const std::string& v)
{
    TypeSys::VarT<std::string>::Set(m_Comp.SetModel(v, m_ExtraMsg));
}


/****************************************/
/*** ComponentModelT::VarModelAnimNrT ***/
/****************************************/

ComponentModelT::VarModelAnimNrT::VarModelAnimNrT(const char* Name, const int& Value, const char* Flags[], ComponentModelT& Comp)
    : TypeSys::VarT<int>(Name, Value, Flags),
      m_Comp(Comp)
{
}


// The compiler-written copy constructor would copy m_Comp from Var.m_Comp,
// but we must obviously use the reference to the proper parent instance instead.
ComponentModelT::VarModelAnimNrT::VarModelAnimNrT(const VarModelAnimNrT& Var, ComponentModelT& Comp)
    : TypeSys::VarT<int>(Var),
      m_Comp(Comp)
{
}


void ComponentModelT::VarModelAnimNrT::Set(const int& v)
{
    TypeSys::VarT<int>::Set(m_Comp.SetAnimNr(v, 0.0f, true));
}


void ComponentModelT::VarModelAnimNrT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    if (!m_Comp.m_Model) return;

    const ArrayT<CafuModelT::AnimT>& Anims = m_Comp.m_Model->GetAnims();

    Strings.PushBack("none");
    Values.PushBack(-1);

    for (unsigned int AnimNr = 0; AnimNr < Anims.Size(); AnimNr++)
    {
        Strings.PushBack(Anims[AnimNr].Name);
        Values.PushBack(AnimNr);
    }
}


/****************************************/
/*** ComponentModelT::VarModelSkinNrT ***/
/****************************************/

ComponentModelT::VarModelSkinNrT::VarModelSkinNrT(const char* Name, const int& Value, const char* Flags[], ComponentModelT& Comp)
    : TypeSys::VarT<int>(Name, Value, Flags),
      m_Comp(Comp)
{
}


// The compiler-written copy constructor would copy m_Comp from Var.m_Comp,
// but we must obviously use the reference to the proper parent instance instead.
ComponentModelT::VarModelSkinNrT::VarModelSkinNrT(const VarModelSkinNrT& Var, ComponentModelT& Comp)
    : TypeSys::VarT<int>(Var),
      m_Comp(Comp)
{
}


void ComponentModelT::VarModelSkinNrT::GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const
{
    if (!m_Comp.m_Model) return;

    const ArrayT<CafuModelT::SkinT>& Skins = m_Comp.m_Model->GetSkins();

    Strings.PushBack("default");
    Values.PushBack(-1);

    for (unsigned int SkinNr = 0; SkinNr < Skins.Size(); SkinNr++)
    {
        Strings.PushBack(Skins[SkinNr].Name);
        Values.PushBack(SkinNr);
    }
}


/***********************/
/*** ComponentModelT ***/
/***********************/

namespace
{
    const char* FlagsIsModelFileName[] = { "IsModelFileName", NULL };
}


const char* ComponentModelT::DocClass =
    "This component adds a 3D model to its window.";


const cf::TypeSys::VarsDocT ComponentModelT::DocVars[] =
{
    { "Name",      "The file name of the model." },
    { "Animation", "The animation sequence number of the model." },
    { "Skin",      "The skin used for rendering the model." },
    { "Pos",       "The position of the model in world space." },
    { "Scale",     "The scale factor applied to the model coordinates when converted to world space." },
    { "Angles",    "The angles around the axes that determine the orientation of the model in world space." },
    { "CameraPos", "The position of the camera in world space." },
    { NULL, NULL }
};


ComponentModelT::ComponentModelT()
    : ComponentBaseT(),
      m_ModelName("Name", "", FlagsIsModelFileName, *this),
      m_ModelAnimNr("Animation", 0, NULL, *this),
      m_ModelSkinNr("Skin", -1, NULL, *this),   // -1 is the default skin of the model.
      m_ModelPos("Pos", Vector3fT()),
      m_ModelScale("Scale", 1.0f),
      m_ModelAngles("Angles", Vector3fT()),
      m_CameraPos("CameraPos", Vector3fT()),
      m_Model(NULL),
      m_Pose(NULL)
{
    // There is no need to init the NULL members here:
    assert(GetWindow() == NULL);

    FillMemberVars();
}


ComponentModelT::ComponentModelT(const ComponentModelT& Comp)
    : ComponentBaseT(Comp),
      m_ModelName(Comp.m_ModelName, *this),
      m_ModelAnimNr(Comp.m_ModelAnimNr, *this),
      m_ModelSkinNr(Comp.m_ModelSkinNr, *this),
      m_ModelPos(Comp.m_ModelPos),
      m_ModelScale(Comp.m_ModelScale),
      m_ModelAngles(Comp.m_ModelAngles),
      m_CameraPos(Comp.m_CameraPos),
      m_Model(NULL),
      m_Pose(NULL)
{
    // There is no need to init the NULL members here:
    assert(GetWindow() == NULL);

    FillMemberVars();
}


void ComponentModelT::FillMemberVars()
{
    GetMemberVars().Add(&m_ModelName);
    GetMemberVars().Add(&m_ModelAnimNr);
    GetMemberVars().Add(&m_ModelSkinNr);
    GetMemberVars().Add(&m_ModelPos);
    GetMemberVars().Add(&m_ModelScale);
    GetMemberVars().Add(&m_ModelAngles);
    GetMemberVars().Add(&m_CameraPos);
}


ComponentModelT::~ComponentModelT()
{
    delete m_Pose;
    m_Pose = NULL;
}


ComponentModelT* ComponentModelT::Clone() const
{
    return new ComponentModelT(*this);
}


void ComponentModelT::UpdateDependencies(WindowT* Window)
{
    const bool WindowChanged = Window != GetWindow();

    ComponentBaseT::UpdateDependencies(Window);

    // m_Transform = NULL;

    if (WindowChanged)
    {
        delete m_Pose;
        m_Pose  = NULL;
        m_Model = NULL;
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

    if (WindowChanged)
    {
        m_ModelName.Set(m_ModelName.Get());
    }
}


void ComponentModelT::Render() const
{
    if (!m_Pose) return;

    // Limitations:
    // Any meshes (images) in the background of this model should use only materials with "ambientMask d" set.
    MatSys::Renderer->PushMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PushMatrix(MatSys::RendererI::WORLD_TO_VIEW );

    MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, MatrixT());
    MatSys::Renderer->Translate(MatSys::RendererI::MODEL_TO_WORLD, m_ModelPos.Get().x, m_ModelPos.Get().y, m_ModelPos.Get().z);
    MatSys::Renderer->Scale    (MatSys::RendererI::MODEL_TO_WORLD, m_ModelScale.Get());
    if (m_ModelAngles.Get().x!=0) MatSys::Renderer->RotateX(MatSys::RendererI::MODEL_TO_WORLD, m_ModelAngles.Get().x);
    if (m_ModelAngles.Get().y!=0) MatSys::Renderer->RotateY(MatSys::RendererI::MODEL_TO_WORLD, m_ModelAngles.Get().y);
    if (m_ModelAngles.Get().z!=0) MatSys::Renderer->RotateZ(MatSys::RendererI::MODEL_TO_WORLD, m_ModelAngles.Get().z);

    MatSys::Renderer->SetMatrix(MatSys::RendererI::WORLD_TO_VIEW, MatrixT::GetRotateXMatrix(-90.0f));   // Rotate coordinate system axes to Cafu standard.
    MatSys::Renderer->Translate(MatSys::RendererI::WORLD_TO_VIEW, -m_CameraPos.Get().x, -m_CameraPos.Get().y, -m_CameraPos.Get().z);

    const MatrixT ProjectionMatrix = MatrixT::GetProjPerspectiveMatrix(67.5f, 640.0f/480.0f, 10.0f, 10000.0f);
    MatSys::Renderer->SetMatrix(MatSys::RendererI::PROJECTION, ProjectionMatrix);

    MatSys::Renderer->SetCurrentRenderAction(MatSys::RendererI::AMBIENT);
    MatSys::Renderer->SetCurrentAmbientLightColor(1.0f, 1.0f, 1.0f);
    MatSys::Renderer->SetCurrentEyePosition(m_CameraPos.Get().x, m_CameraPos.Get().y, m_CameraPos.Get().z); // Required in some ambient shaders.

    m_Pose->Draw(m_ModelSkinNr.Get(), 0.0f);

    MatSys::Renderer->PopMatrix(MatSys::RendererI::PROJECTION    );
    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->PopMatrix(MatSys::RendererI::WORLD_TO_VIEW );
}


void ComponentModelT::OnClockTickEvent(float t)
{
    ComponentBaseT::OnClockTickEvent(t);

    if (!m_Pose) return;

    m_Pose->GetAnimExpr()->AdvanceTime(t);
}


std::string ComponentModelT::SetModel(const std::string& FileName, std::string& Msg)
{
    // It is possible that this is called (e.g. from a script) for a component that is not yet part of a window.
    if (!GetWindow()) return FileName;

    const CafuModelT* PrevModel = m_Model;

    m_Model = GetWindow()->GetGui().GetGuiResources().GetModel(FileName, Msg);

    // If the model didn't change, there is nothing else to do.
    if (PrevModel == m_Model) return m_Model->GetFileName();

    // Need a new pose and updated parameters for the new model.
    delete m_Pose;
    m_Pose = NULL;

    m_ModelAnimNr.Set(m_ModelAnimNr.Get());
    m_ModelSkinNr.Set(m_ModelSkinNr.Get());

    return m_Model->GetFileName();
}


int ComponentModelT::SetAnimNr(int AnimNr, float BlendTime, bool ForceLoop)
{
    // It is possible that this is called (e.g. from a script) for a component that is not yet part of a window.
    if (!m_Model) return AnimNr;

    IntrusivePtrT<AnimExprStandardT> StdAE = m_Model->GetAnimExprPool().GetStandard(AnimNr, 0.0f);
    StdAE->SetForceLoop(ForceLoop);

    if (!m_Pose)
    {
        m_Pose = new AnimPoseT(*m_Model, StdAE);
    }
    else
    {
        if (BlendTime > 0.0f)
        {
            IntrusivePtrT<AnimExpressionT> BlendFrom = m_Pose->GetAnimExpr();

            m_Pose->SetAnimExpr(m_Model->GetAnimExprPool().GetBlend(BlendFrom, StdAE, BlendTime));
        }
        else
        {
            m_Pose->SetAnimExpr(StdAE);
        }
    }

    return StdAE->GetSequNr();
}


static const cf::TypeSys::MethsDocT META_GetNumAnims =
{
    "GetNumAnims",
    "Returns the number of animation sequences in this model.",
    "number", "()"
};

int ComponentModelT::GetNumAnims(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentModelT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentModelT> >(1);

    if (!Comp->m_Model)
        luaL_error(LuaState, "The component must be added to a window before this function can be called.");

    lua_pushinteger(LuaState, Comp->m_Model->GetAnims().Size());
    return 1;
}


static const cf::TypeSys::MethsDocT META_SetAnim =
{
    "SetAnim",
    "Sets a new animation sequence for the pose of this model.\n"
    "Optionally, there is a blending from the previous sequence over a given time.\n"
    "Also optionally, the \"force loop\" flag for the new sequence can be set.\n"
    "For example: `SetAnim(8, 3.0, true)`",
    "number", "(number anim, number blend_time=0.0, boolean force_loop=false)"
};

int ComponentModelT::SetAnim(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentModelT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentModelT> >(1);

    if (!Comp->m_Model)
        luaL_error(LuaState, "The component must be added to a window before this function can be called.");

    const int   AnimNr    = luaL_checkint(LuaState, 2);
    const float BlendTime = float(luaL_checknumber(LuaState, 3));
    const bool  ForceLoop = lua_isnumber(LuaState, 4) ? (lua_tointeger(LuaState, 4) != 0) : (lua_toboolean(LuaState, 4) != 0);

    lua_pushinteger(LuaState, Comp->SetAnimNr(AnimNr, BlendTime, ForceLoop));
    return 1;
}


static const cf::TypeSys::MethsDocT META_GetNumSkins =
{
    "GetNumSkins",
    "Returns the number of skins in this model.",
    "number", "()"
};

int ComponentModelT::GetNumSkins(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentModelT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentModelT> >(1);

    if (!Comp->m_Model)
        luaL_error(LuaState, "The component must be added to a window before this function can be called.");

    lua_pushinteger(LuaState, Comp->m_Model->GetSkins().Size());
    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "This method returns a readable string representation of this object.",
    "string", "()"
};

int ComponentModelT::toString(lua_State* LuaState)
{
    // ScriptBinderT Binder(LuaState);
    // IntrusivePtrT<ComponentModelT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentModelT> >(1);

    lua_pushfstring(LuaState, "model component");
    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentModelT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentModelT();
}

const luaL_Reg ComponentModelT::MethodsList[] =
{
    { "GetNumAnims", GetNumAnims },
    { "SetAnim",     SetAnim },
    { "GetNumSkins", GetNumSkins },
    { "__tostring",  toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentModelT::DocMethods[] =
{
    META_GetNumAnims,
    META_SetAnim,
    META_GetNumSkins,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentModelT::TypeInfo(GetComponentTIM(), "GuiSys::ComponentModelT", "GuiSys::ComponentBaseT", ComponentModelT::CreateInstance, MethodsList, DocClass, DocMethods, NULL, DocVars);
