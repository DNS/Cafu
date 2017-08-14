/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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

    // Note that the new model must be loaded first (as a "side effect" of Set()).
    // Only then can the anim and skin numbers be indices into anims and skins of the "new" model.
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
void ComponentModelT::VarModelAnimNrT::SetAnimNr(int AnimNr)
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
    float BlendTime = 0.0f;
    bool  ForceLoop = false;

    m_Comp.CallLuaMethod("OnAnimationChange", 0, "i>fb", AnimNr, &BlendTime, &ForceLoop);

    StdAE->SetForceLoop(ForceLoop);

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
    SetAnimNr(v);
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
    "This component adds a 3D model to its entity.\n"
    "Models can be used to add geometric detail to a map. Some models also have ready-made\n"
    "\"GUI fixtures\" where scripted GUIs can be attached that players can interact with.\n"
    "Use the CaWE Model Editor in order to import mesh and animation data for models, and\n"
    "to prepare them for use in game maps.\n";


const cf::TypeSys::VarsDocT ComponentModelT::DocVars[] =
{
    { "Show",        "Whether the model is currently shown (useful with scripts)." },
    { "Name",        "The file name of the model." },
    { "Animation",   "The animation sequence number of the model." },
    { "Skin",        "The skin used for rendering the model." },
    { "Scale",       "The scale factor applied to the model coordinates when converted to world space." },
    { "Gui",         "The file name of the GUI to be used with the models GUI fixtures (if there are any)." },
    { "IsSubmodel",  "Is this model a submodel of another model? If set, the pose of this model is aligned with the first \"non-submodel\" in the entity." },
    { "Is1stPerson", "Is this a 1st-person view model? If `true`, the model is rendered if the world is rendered from *this* entity's perspective. If `false`, the model is rendered when seen from the outside, i.e. in everybody else's view. The default is `false`, because `true` is normally only used with the human player's 1st-person carried weapon models." },
    { NULL, NULL }
};


ComponentModelT::ComponentModelT()
    : ComponentBaseT(),
      m_ModelShow("Show", true),
      m_ModelName("Name", "", FlagsIsModelFileName, *this),
      m_ModelAnimNr("Animation", 0, FlagsDontSerialize, *this),  // Already co-serialized along with m_ModelName.
      m_ModelSkinNr("Skin", -1, FlagsDontSerialize, *this),      // -1 is the default skin of the model. Already co-serialized along with m_ModelName.
      m_ModelScale("Scale", 1.0f),
      m_GuiName("Gui", "", FlagsIsGuiFileName, *this),
      m_IsSubmodel("IsSubmodel", false),
      m_Is1stPerson("Is1stPerson", false),
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
      m_ModelShow(Comp.m_ModelShow),
      m_ModelName(Comp.m_ModelName, *this),
      m_ModelAnimNr(Comp.m_ModelAnimNr, *this),
      m_ModelSkinNr(Comp.m_ModelSkinNr, *this),
      m_ModelScale(Comp.m_ModelScale),
      m_GuiName(Comp.m_GuiName, *this),
      m_IsSubmodel(Comp.m_IsSubmodel),
      m_Is1stPerson(Comp.m_Is1stPerson),
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
    GetMemberVars().Add(&m_ModelShow);
    GetMemberVars().Add(&m_ModelName);
    GetMemberVars().Add(&m_ModelAnimNr);
    GetMemberVars().Add(&m_ModelSkinNr);
    GetMemberVars().Add(&m_ModelScale);
    GetMemberVars().Add(&m_GuiName);
    GetMemberVars().Add(&m_IsSubmodel);
    GetMemberVars().Add(&m_Is1stPerson);
}


ComponentModelT::~ComponentModelT()
{
    m_Gui = NULL;

    delete m_Pose;
    m_Pose = NULL;
}


AnimPoseT* ComponentModelT::GetPose() const
{
    if (m_Model && !m_Pose)
    {
        IntrusivePtrT<AnimExprStandardT> StdAE = m_Model->GetAnimExprPool().GetStandard(m_ModelAnimNr.Get(), 0.0f);

        m_Pose = new AnimPoseT(*m_Model, StdAE);
    }

    return m_Pose;
}


IntrusivePtrT<cf::GuiSys::GuiImplT> ComponentModelT::GetGui() const
{
    if (!m_Model || !m_Model->GetGuiFixtures().Size())
    {
        // Not a model with GUI fixtures.
        assert(m_Gui.IsNull());
        return NULL;
    }

    // If we have a model with GUI fixtures, return a valid GUI instance in any case.
    if (m_Gui != NULL) return m_Gui;

    static const char* FallbackGUI =
        "local gui = ...\n"
        "local Root = gui:new('WindowT', 'Root')\n"
        "gui:SetRootWindow(Root)\n"
        "\n"
        "function Root:OnInit()\n"
        "    self:GetTransform():set('Pos', 0, 0)\n"
        "    self:GetTransform():set('Size', 640, 480)\n"
        "\n"
        "    local c1 = gui:new('ComponentTextT')\n"
        "    c1:set('Text', [=====[%s]=====])\n"    // This is intended for use with e.g. wxString::Format().
        " -- c1:set('Font', 'Fonts/Impact')\n"
        "    c1:set('Scale', 0.6)\n"
        "    c1:set('Padding', 0, 0)\n"
        "    c1:set('Color', 15/255, 49/255, 106/255)\n"
        " -- c1:set('Alpha', 0.5)\n"
        "    c1:set('horAlign', 0)\n"
        "    c1:set('verAlign', 0)\n"
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

    WorldT& World = GetEntity()->GetWorld();

    try
    {
        m_Gui = new cf::GuiSys::GuiImplT(
            World.GetScriptState(),
            World.GetGuiResources());

        if (m_GuiName.Get() == "")
        {
            m_Gui->LoadScript(
                cf::String::Replace(FallbackGUI, "%s", "This is a\nfull-scale sample GUI.\n\n"
                    "Set the 'Gui' property\nof the Model component\nto assign the real GUI."),
                cf::GuiSys::GuiImplT::InitFlag_InlineCode);
        }
        else if (cf::String::EndsWith(m_GuiName.Get(), "_init.cgui"))
        {
            m_Gui->LoadScript(
                cf::String::Replace(FallbackGUI, "%s", "Please use filename\n" +
                    cf::String::Replace(m_GuiName.Get(), "_init.cgui", "_main.cgui") +
                    "\nrather than\n" + m_GuiName.Get() + "\nfor the GUI to work right."),
                cf::GuiSys::GuiImplT::InitFlag_InlineCode);
        }
        else
        {
            // Set the GUI object's "Model"  field to the related component instance (`this`),
            // and the GUI object's "Entity" field to the related entity instance.
            // Expressed as pseudo code:
            //     gui.Model  = this
            //     gui.Entity = this->GetEntity()
            {
                lua_State*    LuaState = World.GetScriptState().GetLuaState();
                StackCheckerT StackChecker(LuaState);
                ScriptBinderT Binder(LuaState);

                Binder.Push(m_Gui);
                Binder.Push(IntrusivePtrT<ComponentModelT>(const_cast<ComponentModelT*>(this)));
                lua_setfield(LuaState, -2, "Model");
                Binder.Push(IntrusivePtrT<EntityT>(GetEntity()));
                lua_setfield(LuaState, -2, "Entity");
                lua_pop(LuaState, 1);
            }

            m_Gui->LoadScript(m_GuiName.Get());

            // Active status is not really relevant for our Gui that is not managed by the GuiMan,
            // but still make sure that clock tick events are properly propagated to all windows.
            m_Gui->Activate();
            m_Gui->SetMouseCursorSize(40.0f);
        }
    }
    catch (const cf::GuiSys::GuiImplT::InitErrorT& IE)
    {
        // Need a new GuiImplT instance here, as the one allocated above is in unknown state.
        m_Gui = new cf::GuiSys::GuiImplT(
            World.GetScriptState(),
            World.GetGuiResources());

        // This one must not throw again...
        m_Gui->LoadScript(
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
    BoundingBox3fT BB = ComponentBaseT::GetEditorBB();

    if (!GetPose()) return BB;

    // We could return `GetPose()->GetBB()` here, but this bounding-box is permanently changing if the model is
    // animated, which is not ideal for the purposes of the Map Editor. Instead, always return the bounding-box
    // for frame 0 of the current animation sequence.

    // Save the model's true anim expression for later restore.
    IntrusivePtrT<AnimExpressionT> AnimExpr = GetPose()->GetAnimExpr();

    // Temporarily assign a "standard" anim expression at frame 0.
    GetPose()->SetAnimExpr(m_Model->GetAnimExprPool().GetStandard(m_ModelAnimNr.Get(), 0.0f));

    // Pick up the desired bounding-box.
    BB = GetPose()->GetBB();

    // Restore the true anim expression.
    GetPose()->SetAnimExpr(AnimExpr);

    return BB;
}


BoundingBox3fT ComponentModelT::GetCullingBB() const
{
    return GetPose() ? GetPose()->GetBB() : BoundingBox3fT();
}


bool ComponentModelT::Render(bool FirstPersonView, float LodDist) const
{
    if (!m_ModelShow.Get()) return false;
    if (m_Is1stPerson.Get() != FirstPersonView) return false;
    if (!GetPose()) return false;

    MatSys::Renderer->PushMatrix(MatSys::RendererI::MODEL_TO_WORLD);
    MatSys::Renderer->Scale(MatSys::RendererI::MODEL_TO_WORLD, m_ModelScale.Get());

    if (m_IsSubmodel.Get() && GetEntity() != NULL)
    {
        const ArrayT< IntrusivePtrT<ComponentBaseT> >& Components = GetEntity()->GetComponents();

        for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
            if (strcmp(Components[CompNr]->GetName(), "Model") == 0)
            {
                IntrusivePtrT<const ComponentModelT> ModelComp = dynamic_pointer_cast<ComponentModelT>(Components[CompNr]);

                if (ModelComp == NULL) continue;
                if (ModelComp->m_IsSubmodel.Get()) continue;

                GetPose()->SetSuperPose(ModelComp->GetPose());
                break;
            }
    }

    GetPose()->Draw(m_ModelSkinNr.Get(), LodDist);
    GetPose()->SetSuperPose(NULL);

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
    return true;
}


void ComponentModelT::DoSerialize(cf::Network::OutStreamT& Stream) const
{
    const bool HaveGui = GetGui() != NULL;
    Stream << HaveGui;

    if (HaveGui)
    {
        GetGui()->Serialize(Stream);
    }
}


void ComponentModelT::DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    bool HaveGui = false;
    Stream >> HaveGui;

    if (HaveGui)
    {
        GetGui()->Deserialize(Stream, IsIniting);
    }
}


void ComponentModelT::DoServerFrame(float t)
{
    if (GetGui() != NULL)
    {
        GetGui()->DistributeClockTickEvents(t);
    }

    // Advance the time of anim expressions on the server-side, too, so that
    //   - we get OnSequenceWrap() callbacks in the script code, and
    //   - blending expressions (and similar) can complete and eventually clean-up
    //     their subexpressions that become unused.
    // Note that in the past, we used to test the bare m_Pose rather than GetPose() in the if-condition below,
    // in the hope to get away with not instantiating the pose here, saving resources on the server.
    // However, as our script code relies on calls to OnSequenceWrap(), this now seems hard to avoid.
    if (GetPose())
    {
        IntrusivePtrT<AnimExpressionT> AnimExpr = GetPose()->GetAnimExpr();

        if (AnimExpr->AdvanceTime(t))
        {
            CallLuaMethod("OnSequenceWrap_Sv", 0);
        }
    }
}


void ComponentModelT::DoClientFrame(float t)
{
    if (GetPose())
    {
        IntrusivePtrT<AnimExpressionT> AnimExpr = GetPose()->GetAnimExpr();

        if (AnimExpr->AdvanceTime(t))
        {
            CallLuaMethod("OnSequenceWrap", 0);
        }
    }
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


static const cf::TypeSys::MethsDocT META_GetGui =
{
    "GetGui",
    "Returns the (first) GUI of this model.",
    "GuiImplT", "()"
};

int ComponentModelT::GetGui(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<ComponentModelT> Comp = Binder.GetCheckedObjectParam< IntrusivePtrT<ComponentModelT> >(1);

    if (Comp->GetGui() != NULL)
    {
        Binder.Push(Comp->GetGui());
    }
    else
    {
        lua_pushnil(LuaState);
    }

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
    { "GetNumSkins", GetNumSkins },
    { "GetGui",      GetGui },
    { "__tostring",  toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentModelT::DocMethods[] =
{
    META_GetNumAnims,
    META_GetNumSkins,
    META_GetGui,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::MethsDocT ComponentModelT::DocCallbacks[] =
{
    { "OnAnimationChange",
      "This method is called when a new animation sequence number is set for this model.",
      "tuple", "(int AnimNr)" },
    { "OnSequenceWrap_Sv",
      "This method is called when playing the model's current animation sequence \"wraps\".",
      "", "" },
    { "OnSequenceWrap",
      "This method is called when playing the model's current animation sequence \"wraps\".",
      "", "" },
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT ComponentModelT::TypeInfo(
    GetComponentTIM(),
    "GameSys::ComponentModelT",
    "GameSys::ComponentBaseT",
    ComponentModelT::CreateInstance,
    MethodsList,
    DocClass,
    DocMethods,
    DocCallbacks,
    DocVars);
