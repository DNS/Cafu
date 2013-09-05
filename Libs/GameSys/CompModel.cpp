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

#include "CompModel.hpp"
#include "AllComponents.hpp"
#include "Entity.hpp"
#include "World.hpp"

#include "GuiSys/GuiImpl.hpp"
#include "GuiSys/Window.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/Model_cmdl.hpp"
#include "Models/ModelManager.hpp"
#include "Network/State.hpp"
#include "String.hpp"
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

using namespace cf::GameSys;


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


// The deserialization of network messages on the client can cause a member variable
// of a component to be `Set()` very frequently, and often to the same value as before.
//
// Consequently, setting a variable to the same value must be dealt with as efficiently
// as possible (for performance), and free of unwanted side effects (for correctness).
void ComponentModelT::VarModelNameT::Set(const std::string& v)
{
    // No change? Then there is no need to update the related m_Model resource.
    if (Get() == v) return;

    TypeSys::VarT<std::string>::Set(v);

    m_Comp.ReInit(&m_ExtraMsg);
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


// The deserialization of network messages on the client can cause a member variable
// of a component to be `Set()` very frequently, and often to the same value as before.
//
// Consequently, setting a variable to the same value must be dealt with as efficiently
// as possible (for performance), and free of unwanted side effects (for correctness).
void ComponentModelT::VarModelAnimNrT::SetAnimNr(int AnimNr, float BlendTime, bool ForceLoop)
{
    // Is the "not-normalized" value unchanged?
    // Then there is no need to update the related m_Pose resource (the BlendTime and ForceLoop are ignored).
    // Also, for correctness, we must not cause the m_Pose's anim expression to be reset to frame 0.
    if (Get() == AnimNr) return;

    // It is possible that this is called (e.g. from a script) for a component that is not yet part of an entity.
    // In this case, there is no model instance and no way to normalize AnimNr, so we just have to take it as-is.
    if (!m_Comp.m_Model)
    {
        TypeSys::VarT<int>::Set(AnimNr);
        return;
    }

    // "Normalize" AnimNr.
    IntrusivePtrT<AnimExprStandardT> StdAE = m_Comp.m_Model->GetAnimExprPool().GetStandard(AnimNr, 0.0f);
    StdAE->SetForceLoop(ForceLoop);

    // Is the "normalized" value unchanged?
    // Then there is no need to update the related m_Pose resource (the BlendTime and ForceLoop are ignored).
    // Also, for correctness, we must not cause the m_Pose's anim expression to be reset to frame 0.
    if (Get() == StdAE->GetSequNr()) return;

    // Store the "normalized" value, then update the related m_Pose resource.
    TypeSys::VarT<int>::Set(StdAE->GetSequNr());

    // If there is for some reason no m_Pose yet, then there is no reason to create one now.
    // Leave it up to the ComponentModelT user code to create one as required.
    if (!m_Comp.m_Pose) return;

    // There is a m_Pose, so update it.
    if (BlendTime > 0.0f)
    {
        IntrusivePtrT<AnimExpressionT> BlendFrom = m_Comp.m_Pose->GetAnimExpr();

        m_Comp.m_Pose->SetAnimExpr(m_Comp.m_Model->GetAnimExprPool().GetBlend(BlendFrom, StdAE, BlendTime));
    }
    else
    {
        m_Comp.m_Pose->SetAnimExpr(StdAE);
    }
}


void ComponentModelT::VarModelAnimNrT::Set(const int& v)
{
    SetAnimNr(v, 0.0f, true);
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


/************************************/
/*** ComponentModelT::VarGuiNameT ***/
/************************************/

ComponentModelT::VarGuiNameT::VarGuiNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentModelT& Comp)
    : TypeSys::VarT<std::string>(Name, Value, Flags),
      m_Comp(Comp)
{
}


// The compiler-written copy constructor would copy m_Comp from Var.m_Comp,
// but we must obviously use the reference to the proper parent instance instead.
ComponentModelT::VarGuiNameT::VarGuiNameT(const VarGuiNameT& Var, ComponentModelT& Comp)
    : TypeSys::VarT<std::string>(Var),
      m_Comp(Comp)
{
}


// The deserialization of network messages on the client can cause a member variable
// of a component to be `Set()` very frequently, and often to the same value as before.
//
// Consequently, setting a variable to the same value must be dealt with as efficiently
// as possible (for performance), and free of unwanted side effects (for correctness).
void ComponentModelT::VarGuiNameT::Set(const std::string& v)
{
    // No change? Then there is no need to update the related m_Gui resource.
    if (Get() == v) return;

    // The GUI name has changed.
    TypeSys::VarT<std::string>::Set(v);

    // Simply invalidate the related m_Gui instance and leave it up to the ComponentModelT
    // user code to re-instantiate it as required.
    delete m_Comp.m_Gui;
    m_Comp.m_Gui = NULL;
}


/***********************/
/*** ComponentModelT ***/
/***********************/

namespace
{
    const char* FlagsIsModelFileName[] = { "IsModelFileName", NULL };
    const char* FlagsIsGuiFileName[]   = { "IsGuiFileName",   NULL };
    const char* FlagsDontSerialize[]   = { "DontSerialize",   NULL };
}


const char* ComponentModelT::DocClass =
    "This component adds a 3D model to its entity.";


const cf::TypeSys::VarsDocT ComponentModelT::DocVars[] =
{
    { "Name",      "The file name of the model." },
    { "Animation", "The animation sequence number of the model." },
    { "Skin",      "The skin used for rendering the model." },
    { "Scale",     "The scale factor applied to the model coordinates when converted to world space." },
    { "Gui",       "The file name of the GUI to be used with the models GUI fixtures (if there are any)." },
    { NULL, NULL }
};


ComponentModelT::ComponentModelT()
    : ComponentBaseT(),
      m_ModelName("Name", "", FlagsIsModelFileName, *this),
      m_ModelAnimNr("Animation", 0, FlagsDontSerialize, *this),  // Already co-serialized along with m_ModelName.
      m_ModelSkinNr("Skin", -1, FlagsDontSerialize, *this),      // -1 is the default skin of the model. Already co-serialized along with m_ModelName.
      m_ModelScale("Scale", 1.0f),
      m_GuiName("Gui", "", FlagsIsGuiFileName, *this),
      m_Model(NULL),
      m_Pose(NULL),
      m_Gui(NULL)
{
    // There is no need to init the NULL members here:
    assert(GetEntity() == NULL);

    FillMemberVars();
}


ComponentModelT::ComponentModelT(const ComponentModelT& Comp)
    : ComponentBaseT(Comp),
      m_ModelName(Comp.m_ModelName, *this),
      m_ModelAnimNr(Comp.m_ModelAnimNr, *this),
      m_ModelSkinNr(Comp.m_ModelSkinNr, *this),
      m_ModelScale(Comp.m_ModelScale),
      m_GuiName(Comp.m_GuiName, *this),
      m_Model(NULL),
      m_Pose(NULL),
      m_Gui(NULL)
{
    // There is no need to init the NULL members here:
    assert(GetEntity() == NULL);

    FillMemberVars();
}


void ComponentModelT::FillMemberVars()
{
    GetMemberVars().Add(&m_ModelName);
    GetMemberVars().Add(&m_ModelAnimNr);
    GetMemberVars().Add(&m_ModelSkinNr);
    GetMemberVars().Add(&m_ModelScale);
    GetMemberVars().Add(&m_GuiName);
}


ComponentModelT::~ComponentModelT()
{
    delete m_Gui;
    m_Gui = NULL;

    delete m_Pose;
    m_Pose = NULL;
}


AnimPoseT* ComponentModelT::GetPose() const
{
    if (m_Model && !m_Pose)
    {
        IntrusivePtrT<AnimExprStandardT> StdAE = m_Model->GetAnimExprPool().GetStandard(m_ModelAnimNr.Get(), 0.0f);
        StdAE->SetForceLoop(true);

        m_Pose = new AnimPoseT(*m_Model, StdAE);
    }

    return m_Pose;
}


cf::GuiSys::GuiImplT* ComponentModelT::GetGui() const
{
    if (!m_Model || !m_Model->GetGuiFixtures().Size())
    {
        // Not a model with GUI fixtures.
        assert(!m_Gui);
        return NULL;
    }

    // If we have a model with GUI fixtures, return a valid GUI instance in any case.
    if (m_Gui) return m_Gui;

    static const char* FallbackGUI =
        "Root = gui:new('WindowT', 'Root')\n"
        "gui:SetRootWindow(Root)\n"
        "\n"
        "function Root:OnInit()\n"
        "    self:GetTransform():set('Pos', 0, 0)\n"
        "    self:GetTransform():set('Size', 640, 480)\n"
        "\n"
        "    local c1 = gui:new('ComponentTextT')\n"
        "    c1:set('Text', [=====[%s]=====])\n"    // This is indended for use with e.g. wxString::Format().
        " -- c1:set('Font', 'Fonts/Impact')\n"
        "    c1:set('Scale', 0.6)\n"
        "    c1:set('Padding', 0, 0)\n"
        "    c1:set('Color', 15/255, 49/255, 106/255)\n"
        " -- c1:set('Alpha', 0.5)\n"
        "    c1:set('hor. Align', 0)\n"
        "    c1:set('ver. Align', 0)\n"
        "\n"
        "    local c2 = gui:new('ComponentImageT')\n"
        "    c2:set('Material', '')\n"
        "    c2:set('Color', 150/255, 170/255, 204/255)\n"
        "    c2:set('Alpha', 0.8)\n"
        "\n"
        "    self:AddComponent(c1, c2)\n"
        "\n"
        "    gui:activate      (true)\n"
        "    gui:setInteractive(true)\n"
        "    gui:showMouse     (false)\n"
        "    gui:setFocus      (Root)\n"
        "end\n";

    try
    {
        if (m_GuiName.Get() == "")
        {
            m_Gui = new cf::GuiSys::GuiImplT(
                GetEntity()->GetWorld().GetGuiResources(),
                cf::String::Replace(FallbackGUI, "%s", "This is a\nfull-scale sample GUI.\n\n"
                    "Set the 'Gui' property\nof the Model component\nto assign the real GUI."),
                cf::GuiSys::GuiImplT::InitFlag_InlineCode);
        }
        else
        {
            m_Gui = new cf::GuiSys::GuiImplT(GetEntity()->GetWorld().GetGuiResources(), m_GuiName.Get());

            // Active status is not really relevant for our Gui that is not managed by the GuiMan,
            // but still make sure that clock tick events are properly propagated to all windows.
            m_Gui->Activate();
            m_Gui->SetMouseCursorSize(40.0f);
        }
    }
    catch (const cf::GuiSys::GuiImplT::InitErrorT& IE)
    {
        // This one must not throw again...
        m_Gui = new cf::GuiSys::GuiImplT(
            GetEntity()->GetWorld().GetGuiResources(),
            cf::String::Replace(FallbackGUI, "%s", "Could not load GUI\n" + m_GuiName.Get() + "\n\n" + IE.what()),
            cf::GuiSys::GuiImplT::InitFlag_InlineCode);
    }

    return m_Gui;
}


ComponentModelT* ComponentModelT::Clone() const
{
    return new ComponentModelT(*this);
}


void ComponentModelT::ReInit(std::string* ErrorMsg)
{
    // It is possible that this is called (e.g. from a script) for a component that is not yet part of an entity.
    if (!GetEntity())
    {
        delete m_Gui;
        m_Gui = NULL;

        delete m_Pose;
        m_Pose = NULL;

        m_Model = NULL;
        return;
    }

    const CafuModelT* NewModel = GetEntity()->GetWorld().GetModelMan().GetModel(m_ModelName.Get(), ErrorMsg);

    // If the model didn't change (e.g. because it is the same substitute for the old and the new model),
    // there is nothing else to do. Note that possibly `m_Model->GetFileName() != v` here, because
    // `m_Model->GetFileName()` may be the filename of the first dlod sub-model, not that of the dlod model itself.
    if (NewModel == m_Model) return;

    // The new model may or may not have GUI fixtures, so make sure that the GUI instance is reset.
    delete m_Gui;
    m_Gui = NULL;

    // Need a new pose and updated parameters for the new model.
    delete m_Pose;
    m_Pose = NULL;

    // Assign the new model instance.
    m_Model = NewModel;

    // Re-normalize values that need to be re-normalized.
    const int PrevAnimNr = m_ModelAnimNr.Get();
    m_ModelAnimNr.Set(-1);
    m_ModelAnimNr.Set(PrevAnimNr);

    const int PrevSkinNr = m_ModelSkinNr.Get();
    m_ModelSkinNr.Set(-1);
    m_ModelSkinNr.Set(PrevSkinNr);
}


void ComponentModelT::UpdateDependencies(EntityT* Entity)
{
    ComponentBaseT::UpdateDependencies(Entity);

    ReInit();
}


BoundingBox3fT ComponentModelT::GetEditorBB() const
{
    return GetPose() ? GetPose()->GetBB() : BoundingBox3fT();
}


void ComponentModelT::Render(float LodDist) const
{
    if (!GetPose()) return;

    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->Scale(MatSys::RendererI::MODEL_TO_WORLD, m_ModelScale.Get());

    GetPose()->Draw(m_ModelSkinNr.Get(), LodDist);

    if (LodDist < 1024.0f && MatSys::Renderer->GetCurrentRenderAction() == MatSys::RendererI::AMBIENT)
    {
        const MatrixT ModelToWorld = MatSys::Renderer->GetMatrix(MatSys::RendererI::MODEL_TO_WORLD);

        for (unsigned long GFNr = 0; GFNr < m_Model->GetGuiFixtures().Size(); GFNr++)
        {
            Vector3fT GuiOrigin;
            Vector3fT GuiAxisX;
            Vector3fT GuiAxisY;

            if (GetPose()->GetGuiPlane(GFNr, GuiOrigin, GuiAxisX, GuiAxisY))
            {
#if 1
                // It's pretty easy to derive this matrix geometrically, see my TechArchive note from 2006-08-22.
                const MatrixT M(GuiAxisX.x / 640.0f, GuiAxisY.x / 480.0f, 0.0f, GuiOrigin.x,
                                GuiAxisX.y / 640.0f, GuiAxisY.y / 480.0f, 0.0f, GuiOrigin.y,
                                GuiAxisX.z / 640.0f, GuiAxisY.z / 480.0f, 0.0f, GuiOrigin.z,
                                               0.0f,                0.0f, 0.0f,        1.0f);

                MatSys::Renderer->SetMatrix(MatSys::RendererI::MODEL_TO_WORLD, ModelToWorld * M);

                GetGui()->Render(true /*zLayerCoating*/);
#else
                MatSys::Renderer->SetCurrentMaterial(Gui->GetDefaultRM());

                // Just a single triangle that indicates the position and orientation of the GUI plane.
                static MatSys::MeshT Tri(MatSys::MeshT::Triangles);
                Tri.Vertices.Overwrite();
                Tri.Vertices.PushBackEmpty(3);

                Tri.Vertices[0].SetOrigin(GuiOrigin         ); Tri.Vertices[0].SetTextureCoord(0.0f, 0.0f); Tri.Vertices[0].SetColor(1, 1, 1, 1.0f);
                Tri.Vertices[1].SetOrigin(GuiOrigin+GuiAxisX); Tri.Vertices[1].SetTextureCoord(1.0f, 0.0f); Tri.Vertices[1].SetColor(1, 0, 0, 0.5f);
                Tri.Vertices[2].SetOrigin(GuiOrigin+GuiAxisY); Tri.Vertices[2].SetTextureCoord(0.0f, 1.0f); Tri.Vertices[2].SetColor(0, 1, 0, 0.5f);

                MatSys::Renderer->RenderMesh(Tri);
#endif
            }
        }
    }

    MatSys::Renderer->PopMatrix(MatSys::RendererI::MODEL_TO_WORLD);
}


void ComponentModelT::OnClockTickEvent(float t)
{
    ComponentBaseT::OnClockTickEvent(t);

    if (GetPose())
        GetPose()->GetAnimExpr()->AdvanceTime(t);

    if (GetGui())
        GetGui()->DistributeClockTickEvents(t);
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
        luaL_error(LuaState, "The component must be added to an entity before this function can be called.");

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
    "", "(number anim, number blend_time=0.0, boolean force_loop=false)"
};

int ComponentModelT::SetAnim(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentModelT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentModelT> >(1);

    if (!Comp->m_Model)
        luaL_error(LuaState, "The component must be added to an entity before this function can be called.");

    const int   AnimNr    = luaL_checkint(LuaState, 2);
    const float BlendTime = float(luaL_checknumber(LuaState, 3));
    const bool  ForceLoop = lua_isnumber(LuaState, 4) ? (lua_tointeger(LuaState, 4) != 0) : (lua_toboolean(LuaState, 4) != 0);

    Comp->m_ModelAnimNr.SetAnimNr(AnimNr, BlendTime, ForceLoop);
    return 0;
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
        luaL_error(LuaState, "The component must be added to an entity before this function can be called.");

    lua_pushinteger(LuaState, Comp->m_Model->GetSkins().Size());
    return 1;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__toString",
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

const luaL_reg ComponentModelT::MethodsList[] =
{
    { "GetNumAnims", ComponentModelT::GetNumAnims },
    { "SetAnim",     ComponentModelT::SetAnim },
    { "GetNumSkins", ComponentModelT::GetNumSkins },
    { "__tostring",  ComponentModelT::toString },
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

const cf::TypeSys::TypeInfoT ComponentModelT::TypeInfo(GetComponentTIM(), "ComponentModelT", "ComponentBaseT", ComponentModelT::CreateInstance, MethodsList, DocClass, DocMethods, DocVars);
