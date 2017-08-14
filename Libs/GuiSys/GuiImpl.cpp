/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "GuiImpl.hpp"
#include "AllComponents.hpp"
#include "CompBase.hpp"
#include "Window.hpp"
#include "WindowCreateParams.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/Console_Lua.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Network/State.hpp"
#include "OpenGL/OpenGLWindow.hpp"  // Just for the Ca*EventT classes...
#include "String.hpp"
#include "TypeSys.hpp"
#include "UniScriptState.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <cassert>
#include <cstring>


using namespace cf::GuiSys;


// Note that we cannot simply replace this method with a global TypeInfoManT instance,
// because it is called during global static initialization time. The TIM instance being
// embedded in the function guarantees that it is properly initialized before first use.
cf::TypeSys::TypeInfoManT& cf::GuiSys::GetGuiTIM()
{
    static cf::TypeSys::TypeInfoManT TIM;

    return TIM;
}


const char* GuiImplT::DocClass =
    "This class holds the hierarchy of windows that together form a GUI.";


GuiImplT::InitErrorT::InitErrorT(const std::string& Message)
    : std::runtime_error(Message)
{
}


/*static*/ void GuiImplT::InitScriptState(cf::UniScriptStateT& ScriptState)
{
    lua_State* LuaState = ScriptState.GetLuaState();

    // Load the console library. (Adds a global table with name "Console" to the LuaState with the functions of the ConsoleI interface.)
    cf::Console_RegisterLua(LuaState);

    // Load the "ci" (console interpreter) library. (Adds a global table with name "ci" to the LuaState with (some of) the functions of the ConsoleInterpreterI interface.)
    ConsoleInterpreterI::RegisterLua(LuaState);

    // For each class that the TypeInfoManTs know about, add a (meta-)table to the registry of the LuaState.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    ScriptBinderT Binder(LuaState);

    Binder.Init(GetGuiTIM());
    Binder.Init(GetWindowTIM());
    Binder.Init(GetComponentTIM());
}


GuiImplT::GuiImplT(cf::UniScriptStateT& ScriptState, GuiResourcesT& GuiRes)
    : ScriptName(),
      m_ScriptState(&ScriptState),
      m_IsOwnScriptSt(false),
      m_MaterialMan(),
      m_GuiDefaultRM(NULL),
      m_GuiPointerRM(NULL),
      m_GuiFinishZRM(NULL),
      m_GuiResources(GuiRes),
      RootWindow(NULL),
      FocusWindow(NULL),
      MouseOverWindow(NULL),
      m_IsInited(false),
      IsActive(true),
      IsInteractive(true),
      IsFullCover(false),
      MousePosX(VIRTUAL_SCREEN_SIZE_X/2.0f),   // 320.0f
      MousePosY(VIRTUAL_SCREEN_SIZE_Y/2.0f),   // 240.0f
      m_MouseCursorSize(20.0f),
      MouseIsShown(true)
{
}


GuiImplT::GuiImplT(GuiResourcesT& GuiRes)
    : ScriptName(),
      m_ScriptState(new UniScriptStateT()),
      m_IsOwnScriptSt(true),
      m_MaterialMan(),
      m_GuiDefaultRM(NULL),
      m_GuiPointerRM(NULL),
      m_GuiFinishZRM(NULL),
      m_GuiResources(GuiRes),
      RootWindow(NULL),
      FocusWindow(NULL),
      MouseOverWindow(NULL),
      m_IsInited(false),
      IsActive(true),
      IsInteractive(true),
      IsFullCover(false),
      MousePosX(VIRTUAL_SCREEN_SIZE_X/2.0f),   // 320.0f
      MousePosY(VIRTUAL_SCREEN_SIZE_Y/2.0f),   // 240.0f
      m_MouseCursorSize(20.0f),
      MouseIsShown(true)
{
    InitScriptState(*m_ScriptState);
}


void GuiImplT::LoadScript(const std::string& GuiScriptName, int Flags)
{
    ScriptName = (Flags & InitFlag_InlineCode) ? "" : GuiScriptName;

    if ((Flags & InitFlag_InlineCode) == 0)
    {
        std::string s=cf::String::StripExt(GuiScriptName);

        if (cf::String::EndsWith(s, "_main") || cf::String::EndsWith(s, "_init"))
            s=std::string(s, 0, s.length()-5);

        /*ArrayT<MaterialT*> AllMats=*/m_MaterialMan.RegisterMaterialScript(s+".cmat", cf::String::GetPath(GuiScriptName)+"/");
    }

    if (!m_MaterialMan.HasMaterial("Gui/Default"))
    {
        // This material has either not been defined in the .cmat file, or we're dealing with inline code.
        MaterialT Mat;

        Mat.Name          ="Gui/Default";
        Mat.UseMeshColors =true;
        Mat.BlendFactorSrc=MaterialT::SrcAlpha;
        Mat.BlendFactorDst=MaterialT::OneMinusSrcAlpha;
        Mat.AmbientMask[4]=false;

        m_MaterialMan.RegisterMaterial(Mat);
    }

    if (!m_MaterialMan.HasMaterial("Gui/Cursors/Pointer"))
    {
        // This material has either not been defined in the .cmat file, or we're dealing with inline code.
        MaterialT Mat;

        Mat.Name          ="Gui/Cursors/Pointer";
     // Mat.DiffMapComp   =...;
        Mat.UseMeshColors =true;
        Mat.BlendFactorSrc=MaterialT::SrcAlpha;
        Mat.BlendFactorDst=MaterialT::OneMinusSrcAlpha;

        m_MaterialMan.RegisterMaterial(Mat);
    }

    if (!m_MaterialMan.HasMaterial("Gui/FinishZ"))
    {
        // This material has either not been defined in the .cmat file, or we're dealing with inline code.
        MaterialT Mat;

        Mat.Name          ="Gui/FinishZ";
     // Mat.DiffMapComp   =...;
        Mat.UseMeshColors =true;
        Mat.AmbientMask[0]=false;
        Mat.AmbientMask[1]=false;
        Mat.AmbientMask[2]=false;
        Mat.AmbientMask[3]=false;

        m_MaterialMan.RegisterMaterial(Mat);
    }

    MatSys::Renderer->FreeMaterial(m_GuiDefaultRM);    // It shouldn't happen, but LoadScript() might still be called multiple times.
    MatSys::Renderer->FreeMaterial(m_GuiPointerRM);
    MatSys::Renderer->FreeMaterial(m_GuiFinishZRM);

    m_GuiDefaultRM = MatSys::Renderer->RegisterMaterial(m_MaterialMan.GetMaterial("Gui/Default"));
    m_GuiPointerRM = MatSys::Renderer->RegisterMaterial(m_MaterialMan.GetMaterial("Gui/Cursors/Pointer"));
    m_GuiFinishZRM = MatSys::Renderer->RegisterMaterial(m_MaterialMan.GetMaterial("Gui/FinishZ"));


    const std::string PrintScriptName((Flags & InitFlag_InlineCode) ? "<inline code>" : GuiScriptName);
    lua_State*        LuaState = GetScriptState().GetLuaState();
    ScriptBinderT     Binder(LuaState);

    // Load the user script!
    const int LoadResult = (Flags & InitFlag_InlineCode) ? luaL_loadstring(LuaState, GuiScriptName.c_str())
                                                         : luaL_loadfile  (LuaState, GuiScriptName.c_str());

    if (LoadResult != 0)
    {
        const std::string Msg = "Could not load \"" + PrintScriptName + "\":\n" + lua_tostring(LuaState, -1);

        lua_pop(LuaState, 1);
        Console->Warning(Msg + "\n");

        // The LuaState will be closed by the m_ScriptState.
        throw InitErrorT(Msg);
    }

    // This is the parameter for the lua_pcall().
    // Script code will fetch it via the "..." ellipsis operator like this:
    //     local gui = ...
    Binder.Push(IntrusivePtrT<GuiImplT>(this));

    if (lua_pcall(LuaState, 1, 0, 0) != 0)
    {
        const std::string Msg = "Could not load \"" + PrintScriptName + "\":\n" + lua_tostring(LuaState, -1);

        lua_pop(LuaState, 1);
        Console->Warning(Msg + "\n");

        // The LuaState will be closed by the m_ScriptState.
        throw InitErrorT(Msg);
    }

    if (RootWindow == NULL)
    {
        const std::string Msg = "No root window set for GUI \"" + PrintScriptName + "\".";

        Console->Warning(Msg + "\n");

        // The LuaState will be closed by the m_ScriptState.
        throw InitErrorT(Msg);
    }

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);


    // Finally call the Lua OnInit() and OnInit2() methods of each window.
    ArrayT< IntrusivePtrT<WindowT> > AllChildren;

    AllChildren.PushBack(RootWindow);
    RootWindow->GetChildren(AllChildren, true);

    Init();     // The script has the option to call this itself (via gui:Init()) at an earlier time.

    for (unsigned long ChildNr=0; ChildNr<AllChildren.Size(); ChildNr++)
    {
        // The OnInit2() methods contain custom, hand-written code by the user (*_main.cgui files).
        AllChildren[ChildNr]->CallLuaMethod("OnInit2");

        // Let each component know that the "static" part of initialization is now complete.
        const ArrayT< IntrusivePtrT<ComponentBaseT> >& Components = AllChildren[ChildNr]->GetComponents();

        for (unsigned int CompNr = 0; CompNr < Components.Size(); CompNr++)
            Components[CompNr]->OnPostLoad((Flags & InitFlag_InGuiEditor) != 0);
    }


    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);
}


GuiImplT::~GuiImplT()
{
    // Manually "destruct" these references before the Lua state (m_ScriptState).
    // This is redundant: the normal member destruction sequence achieves the same.
    RootWindow=NULL;
    FocusWindow=NULL;
    MouseOverWindow=NULL;

    // Free the render materials.
    MatSys::Renderer->FreeMaterial(m_GuiDefaultRM);
    MatSys::Renderer->FreeMaterial(m_GuiPointerRM);
    MatSys::Renderer->FreeMaterial(m_GuiFinishZRM);

    if (m_IsOwnScriptSt)
    {
        delete m_ScriptState;
        m_ScriptState = NULL;
    }
}


void GuiImplT::ObsoleteForceKill()
{
    assert(m_IsOwnScriptSt);

    if (m_IsOwnScriptSt)
    {
        delete m_ScriptState;
        m_ScriptState = NULL;
    }
}


void GuiImplT::Init()
{
    if (m_IsInited) return;

    ArrayT< IntrusivePtrT<WindowT> > AllChildren;

    AllChildren.PushBack(RootWindow);
    RootWindow->GetChildren(AllChildren, true);

    for (unsigned long ChildNr = 0; ChildNr < AllChildren.Size(); ChildNr++)
    {
        // The OnInit() methods are automatically written by the Cafu GUI editor (*_init.cgui files).
        AllChildren[ChildNr]->CallLuaMethod("OnInit");
    }

    m_IsInited = true;
}


MatSys::RenderMaterialT* GuiImplT::GetDefaultRM() const
{
    return m_GuiDefaultRM;
}


MatSys::RenderMaterialT* GuiImplT::GetPointerRM() const
{
    return m_GuiPointerRM;
}


const std::string& GuiImplT::GetScriptName() const
{
    return ScriptName;
}


IntrusivePtrT<WindowT> GuiImplT::GetRootWindow() const
{
    return RootWindow;
}


IntrusivePtrT<WindowT> GuiImplT::GetFocusWindow() const
{
    return FocusWindow;
}


void GuiImplT::Activate(bool doActivate)
{
    IsActive=doActivate;

    // Call the OnActivate() or OnDeactivate() methods of all windows.
    ArrayT< IntrusivePtrT<WindowT> > AllChildren;

    AllChildren.PushBack(RootWindow);
    RootWindow->GetChildren(AllChildren, true);

    for (unsigned long ChildNr=0; ChildNr<AllChildren.Size(); ChildNr++)
        AllChildren[ChildNr]->CallLuaMethod(IsActive ? "OnActivate" : "OnDeactivate");
}


void GuiImplT::SetInteractive(bool IsInteractive_)
{
    IsInteractive=IsInteractive_;
}


void GuiImplT::GetMousePos(float& MousePosX_, float& MousePosY_) const
{
    MousePosX_=MousePosX;
    MousePosY_=MousePosY;
}


void GuiImplT::SetMousePos(float MousePosX_, float MousePosY_)
{
    MousePosX=MousePosX_;
    MousePosY=MousePosY_;


    // Clip the mouse position to valid coordinates.
    if (MousePosX<0.0f) MousePosX=0.0;
    if (MousePosX>VIRTUAL_SCREEN_SIZE_X) MousePosX=VIRTUAL_SCREEN_SIZE_X;

    if (MousePosY<0.0f) MousePosY=0.0;
    if (MousePosY>VIRTUAL_SCREEN_SIZE_Y) MousePosY=VIRTUAL_SCREEN_SIZE_Y;


    // Determine if the mouse cursor has been moved into (or "over") another window,
    // that is, see if we have to run any OnMouseLeave() and OnMouseEnter() scripts.
    IntrusivePtrT<WindowT> Win=RootWindow->Find(Vector2fT(MousePosX, MousePosY));

    if (Win != MouseOverWindow)
    {
        if (MouseOverWindow != NULL) MouseOverWindow->CallLuaMethod("OnMouseLeave");
        MouseOverWindow = Win;
        if (MouseOverWindow != NULL) MouseOverWindow->CallLuaMethod("OnMouseEnter");
    }
}


void GuiImplT::SetShowMouse(bool ShowMouse_)
{
    MouseIsShown=ShowMouse_;
}


// Note that this method is the twin of Deserialize(), whose implementation it must match.
void GuiImplT::Serialize(cf::Network::OutStreamT& Stream) const
{
    assert(m_IsInited);

    Stream << MousePosX;
    Stream << MousePosY;
    Stream << m_MouseCursorSize;

    RootWindow->Serialize(Stream, true /*WithChildren*/);
}


// Note that this method is the twin of Serialize(), whose implementation it must match.
void GuiImplT::Deserialize(cf::Network::InStreamT& Stream, bool IsIniting)
{
    assert(m_IsInited);

    Stream >> MousePosX;
    Stream >> MousePosY;
    Stream >> m_MouseCursorSize;

    RootWindow->Deserialize(Stream, IsIniting);
}


void GuiImplT::Render(bool zLayerCoating) const
{
    RootWindow->Render();

    static MatSys::MeshT Mesh(MatSys::MeshT::TriangleFan);

    if (Mesh.Vertices.Size()<4)
    {
        Mesh.Vertices.PushBackEmpty(4);

        for (unsigned long VNr=0; VNr<Mesh.Vertices.Size(); VNr++)
        {
            Mesh.Vertices[VNr].SetColor(1.0f, 1.0f, 1.0f, 1.0f);
            Mesh.Vertices[VNr].SetTextureCoord(VNr==0 || VNr==3 ? 0.0f : 1.0f, VNr<2 ? 0.0f : 1.0f);
        }
    }

    if (MouseIsShown)
    {
        Mesh.Vertices[0].SetOrigin(MousePosX,                   MousePosY                  );
        Mesh.Vertices[1].SetOrigin(MousePosX+m_MouseCursorSize, MousePosY                  );
        Mesh.Vertices[2].SetOrigin(MousePosX+m_MouseCursorSize, MousePosY+m_MouseCursorSize);
        Mesh.Vertices[3].SetOrigin(MousePosX,                   MousePosY+m_MouseCursorSize);

        MatSys::Renderer->SetCurrentMaterial(m_GuiPointerRM);
        MatSys::Renderer->RenderMesh(Mesh);
    }

    if (zLayerCoating)
    {
        // Finish by applying a z-layer coating to the GUI screen.
        // This is important whenever the z-ordering of scene elements can be imperfect, e.g. in the Map Editor.
        Mesh.Vertices[0].SetOrigin(  0.0f,   0.0f);
        Mesh.Vertices[1].SetOrigin(640.0f,   0.0f);
        Mesh.Vertices[2].SetOrigin(640.0f, 480.0f);
        Mesh.Vertices[3].SetOrigin(  0.0f, 480.0f);

        MatSys::Renderer->SetCurrentMaterial(m_GuiFinishZRM);
        MatSys::Renderer->RenderMesh(Mesh);
    }
}


bool GuiImplT::ProcessDeviceEvent(const CaKeyboardEventT& KE)
{
    for (IntrusivePtrT<WindowT> Win=FocusWindow; Win!=NULL; Win=Win->GetParent())
    {
        bool KeyWasProcessed=false;
        bool ResultOK       =false;

        switch (KE.Type)
        {
            case CaKeyboardEventT::CKE_KEYDOWN: ResultOK=Win->CallLuaMethod("OnKeyPress",   "i>b", KE.Key, &KeyWasProcessed); break;
            case CaKeyboardEventT::CKE_CHAR:    ResultOK=Win->CallLuaMethod("OnChar",       "i>b", KE.Key, &KeyWasProcessed); break;
            case CaKeyboardEventT::CKE_KEYUP:   ResultOK=Win->CallLuaMethod("OnKeyRelease", "i>b", KE.Key, &KeyWasProcessed); break;
        }

        if (ResultOK && KeyWasProcessed) return true;
        if (Win->OnInputEvent(KE)) return true;
    }

    return false;
}


bool GuiImplT::ProcessDeviceEvent(const CaMouseEventT& ME)
{
    // Note that the processing of the mouse event is orthogonal to (independent of) MouseIsShown.
    // That is, we may be active and interactive even if the mouse cursor is *not* shown,
    // as for example with the 3D window of the Cafu engine client, which must receive the
    // mouse events even if no mouse cursor is shown!

    bool MEWasProcessed=false;      // If the Lua script handler consumed the event.
    bool ResultOK      =false;      // If the Lua script handler returned without script error.

    switch (ME.Type)
    {
        case CaMouseEventT::CM_BUTTON0:
        case CaMouseEventT::CM_BUTTON1:
        case CaMouseEventT::CM_BUTTON2:
        case CaMouseEventT::CM_BUTTON3:
        {
            const bool ButtonDown=(ME.Amount>0);    // Was it a "button up" or "button down" event?

            if (MouseOverWindow!=NULL)
            {
                ResultOK=MouseOverWindow->CallLuaMethod(ButtonDown ? "OnMouseButtonDown" : "OnMouseButtonUp", "i>b", ME.Type, &MEWasProcessed);
            }

            // Change the keyboard input focus, but only if the mouse button event was not handled by the script
            // (by handling the event the script signals that the default behaviour (focus change) should not take place).
            if (!MEWasProcessed && ME.Type==CaMouseEventT::CM_BUTTON0 && ButtonDown && MouseOverWindow!=FocusWindow)
            {
                // Only "unhandled left mouse button down" events change the keyboard input focus.
                if (FocusWindow!=NULL) FocusWindow->CallLuaMethod("OnFocusLose");
                FocusWindow=MouseOverWindow;
                if (FocusWindow!=NULL) FocusWindow->CallLuaMethod("OnFocusGain");
            }

            break;
        }

        case CaMouseEventT::CM_MOVE_X:
        {
            // Update the mouse cursor position according to the ME.
            SetMousePos(MousePosX+ME.Amount, MousePosY);

            // if (MouseOverWindow!=NULL)
            // {
            //     ResultOK=MouseOverWindow->CallLuaMethod("OnMouseMove", "ii>b", RelMousePosX, RelMousePosY, &MEWasProcessed);
            // }
            break;
        }

        case CaMouseEventT::CM_MOVE_Y:
        {
            // Update the mouse cursor position according to the ME.
            SetMousePos(MousePosX, MousePosY+ME.Amount);

            // if (MouseOverWindow!=NULL)
            // {
            //     ResultOK=MouseOverWindow->CallLuaMethod("OnMouseMove", "ii>b", RelMousePosX, RelMousePosY, &MEWasProcessed);
            // }
            break;
        }

        default:
            // Just ignore the other possible ME types.
            break;
    }

    if (ResultOK && MEWasProcessed) return true;
    if (MouseOverWindow==NULL) return false;

    const Vector2fT AbsWinPos = MouseOverWindow->GetAbsolutePos();

    return MouseOverWindow->OnInputEvent(ME, MousePosX-AbsWinPos.x, MousePosY-AbsWinPos.y);
}


void GuiImplT::DistributeClockTickEvents(float t)
{
    if (GetIsActive())  // Inconsistent and inconsequent, but saves performance: Only distribute clock tick events when this GUI is active.
    {
        ArrayT< IntrusivePtrT<WindowT> > AllChildren;

        AllChildren.PushBack(RootWindow);
        RootWindow->GetChildren(AllChildren, true);

        for (unsigned long ChildNr=0; ChildNr<AllChildren.Size(); ChildNr++)
        {
            AllChildren[ChildNr]->OnClockTickEvent(t);
            AllChildren[ChildNr]->CallLuaMethod("OnFrame");
        }
    }

    if (m_IsOwnScriptSt)
    {
        // Run the pending coroutines always, even if this GUI is currently not active.
        m_ScriptState->RunPendingCoroutines(t);
    }
}


/***********************************************/
/*** Implementation of Lua binding functions ***/
/***********************************************/

static const cf::TypeSys::MethsDocT META_Activate =
{
    "activate",
    "Sets the IsActive flag of this GUI.",
    "", "(bool IsActive)"
};

int GuiImplT::Activate(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    // I also want to treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(LuaState, 2)) Gui->IsActive=lua_tointeger(LuaState, 2)!=0;
                              else Gui->IsActive=lua_toboolean(LuaState, 2)!=0;

    return 0;
}


static const cf::TypeSys::MethsDocT META_Close =
{
    "close",
    "Same as calling `gui:activate(false);`",
    "", "()"
};

int GuiImplT::Close(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    Gui->IsActive=false;
    return 0;
}


static const cf::TypeSys::MethsDocT META_SetInteractive =
{
    "setInteractive",
    "Sets the IsInteractive flag of this GUI.",
    "", "(bool IsInteractive)"
};

int GuiImplT::SetInteractive(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    // I also want to treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(LuaState, 2)) Gui->IsInteractive=lua_tointeger(LuaState, 2)!=0;
                              else Gui->IsInteractive=lua_toboolean(LuaState, 2)!=0;

    return 0;
}


static const cf::TypeSys::MethsDocT META_SetFullCover =
{
    "setFullCover",
    "Sets the IsFullCover flag of this GUI.",
    "", "(bool IsFullCover)"
};

int GuiImplT::SetFullCover(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    // I also want to treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(LuaState, 2)) Gui->IsFullCover=lua_tointeger(LuaState, 2)!=0;
                              else Gui->IsFullCover=lua_toboolean(LuaState, 2)!=0;

    return 0;
}


static const cf::TypeSys::MethsDocT META_SetMousePos =
{
    "setMousePos",
    "Sets the position of the mouse cursor.",
    "", "(number x, number y)"
};

int GuiImplT::SetMousePos(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    Gui->SetMousePos(float(lua_tonumber(LuaState, 2)),
                     float(lua_tonumber(LuaState, 3)));

    return 0;
}


static const cf::TypeSys::MethsDocT META_SetMouseCursorSize =
{
    "setMouseCursorSize",
    "Sets the size of the mouse cursor.",
    "", "(number size)"
};

int GuiImplT::SetMouseCursorSize(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    Gui->SetMouseCursorSize(float(lua_tonumber(LuaState, 2)));

    return 0;
}


static const cf::TypeSys::MethsDocT META_SetMouseMat =
{
    "setMouseMat",
    "Sets the material that is used to render the mouse cursor.\n"
    "(This method is not yet implemented.)",
    "", "(string MatName)"
};

int GuiImplT::SetMouseMat(lua_State* LuaState)
{
    return 0;
}


static const cf::TypeSys::MethsDocT META_SetMouseIsShown =
{
    "showMouse",
    "Determines whether the mouse cursor is shown at all.",
    "", "(bool IsShown)"
};

int GuiImplT::SetMouseIsShown(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    // I also want to treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(LuaState, 2)) Gui->MouseIsShown=lua_tointeger(LuaState, 2)!=0;
                              else Gui->MouseIsShown=lua_toboolean(LuaState, 2)!=0;

    return 0;
}


static const cf::TypeSys::MethsDocT META_CreateNew =
{
    "new",
    "This method creates new GUI windows or new GUI components.",
    "object", "(string ClassName, string InstanceName=\"\")"
};

int GuiImplT::CreateNew(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    const std::string TypeName = std::string("GuiSys::") + luaL_checkstring(LuaState, 2);
    const char*       ObjName  = lua_tostring(LuaState, 3);   // Passing an object name is optional.

    const cf::TypeSys::TypeInfoT* TI = GetWindowTIM().FindTypeInfoByName(TypeName.c_str());

    if (TI)
    {
        IntrusivePtrT<WindowT> Win(static_cast<WindowT*>(TI->CreateInstance(WindowCreateParamsT(*Gui))));

        // Console->DevPrint(cf::va("Creating window %p.\n", Win));
        assert(Win->GetType() == TI);
        assert(strcmp(TI->ClassName, TypeName.c_str()) == 0);

        if (ObjName) Win->GetBasics()->SetWindowName(ObjName);

        Binder.Push(Win);
        return 1;
    }

    TI = GetComponentTIM().FindTypeInfoByName(TypeName.c_str());

    if (TI)
    {
        IntrusivePtrT<ComponentBaseT> Comp(static_cast<ComponentBaseT*>(TI->CreateInstance(cf::TypeSys::CreateParamsT())));

        Binder.Push(Comp);
        return 1;
    }

    return luaL_argerror(LuaState, 2, (std::string("unknown class name \"") + TypeName + "\"").c_str());
}


static const cf::TypeSys::MethsDocT META_SetFocus =
{
    "setFocus",
    "Sets the keyboard input focus to the given window. Does *not* call the OnFocusLose() or OnFocusGain() callbacks!\n"
    "Instead of a window instance, it is also possible to pass the name of the window (a string).",
    "", "(WindowT Window)"
};

int GuiImplT::SetFocus(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    if (lua_isstring(LuaState, 2))
    {
        Gui->FocusWindow=Gui->RootWindow->Find(lua_tostring(LuaState, 2));
    }
    else if (lua_istable(LuaState, 2))
    {
        Gui->FocusWindow=Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(2);
    }

    // Note that we intentionally did *not* call the Lua OnFocusLose() or OnFocusGain() scripts,
    // as is done when the focus changes due to a mouse click:
    // the Lua code that calls this method should instead call the desired OnFocus*() script functions itself.
    // This behaviour must be explicitly stated in the user documentation!
    return 0;
}


static const cf::TypeSys::MethsDocT META_GetRootWindow =
{
    "GetRootWindow",
    "Returns the root window of this GUI as previously set by SetRootWindow().",
    "WindowT", "()"
};

int GuiImplT::GetRootWindow(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    if (Gui->RootWindow.IsNull())
    {
        lua_pushnil(LuaState);
    }
    else
    {
        Binder.Push(Gui->RootWindow);
    }

    return 1;
}


static const cf::TypeSys::MethsDocT META_SetRootWindow =
{
    "SetRootWindow",
    "Sets the root window for this GUI.",
    "", "(WindowT Window)"
};

int GuiImplT::SetRootWindow(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    Gui->RootWindow = Binder.GetCheckedObjectParam< IntrusivePtrT<WindowT> >(2);
    return 0;
}


static const cf::TypeSys::MethsDocT META_Init =
{
    "Init",
    "Calls the OnInit() script methods of all windows.",
    "", "()"
};

int GuiImplT::Init(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    Gui->Init();
    return 0;
}


static const cf::TypeSys::MethsDocT META_toString =
{
    "__tostring",
    "Returns a short string representation of this GUI.",
    "string", "()"
};

int GuiImplT::toString(lua_State* LuaState)
{
    ScriptBinderT Binder(LuaState);
    IntrusivePtrT<GuiImplT> Gui = Binder.GetCheckedObjectParam< IntrusivePtrT<GuiImplT> >(1);

    if (Gui->ScriptName=="") lua_pushfstring(LuaState, "A programmatically generated GUI.");
                        else lua_pushfstring(LuaState, "A gui generated from the script \"%s\".", Gui->ScriptName.c_str());

    return 1;
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* GuiImplT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    // At this time, GuiImplT instances cannot be created "anonymously" via the TypeSys' CreateInstance() method,
    // as it would require us to derive from cf::TypeSys::CreateParamsT and deal with that.
    // That's not a problem though, because there is no class hierarchy deriving from GuiImplT, so any code that
    // instantiates GuiImplTs can do so by using the normal constructor -- no "virtual constructor" is needed.

    // return new GuiImplT(*static_cast<const cf::GameSys::GuiCreateParamsT*>(&Params));
    return NULL;
}

const luaL_Reg GuiImplT::MethodsList[] =
{
    { "activate",           Activate },
    { "close",              Close },
    { "setInteractive",     SetInteractive },
    { "setFullCover",       SetFullCover },
    { "setMousePos",        SetMousePos },
    { "setMouseCursorSize", SetMouseCursorSize },
    { "setMouseMat",        SetMouseMat },
    { "showMouse",          SetMouseIsShown },
    { "new",                CreateNew },
    { "setFocus",           SetFocus },
    { "GetRootWindow",      GetRootWindow },
    { "SetRootWindow",      SetRootWindow },
    { "Init",               Init },
    { "__tostring",         toString },
    { NULL, NULL }
};

const cf::TypeSys::MethsDocT GuiImplT::DocMethods[] =
{
    META_Activate,
    META_Close,
    META_SetInteractive,
    META_SetFullCover,
    META_SetMousePos,
    META_SetMouseCursorSize,
    META_SetMouseMat,
    META_SetMouseIsShown,
    META_CreateNew,
    META_SetFocus,
    META_GetRootWindow,
    META_SetRootWindow,
    META_Init,
    META_toString,
    { NULL, NULL, NULL, NULL }
};

const cf::TypeSys::TypeInfoT GuiImplT::TypeInfo(GetGuiTIM(), "GuiSys::GuiImplT", NULL /*No base class.*/, CreateInstance, MethodsList, DocClass, DocMethods);
