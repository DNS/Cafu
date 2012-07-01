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

#include "GuiImpl.hpp"
#include "Window.hpp"
#include "WindowCreateParams.hpp"
#include "ConsoleCommands/Console.hpp"
#include "ConsoleCommands/Console_Lua.hpp"
#include "ConsoleCommands/ConsoleInterpreter.hpp"
#include "Fonts/FontTT.hpp"
#include "MaterialSystem/Material.hpp"
#include "MaterialSystem/Mesh.hpp"
#include "MaterialSystem/Renderer.hpp"
#include "Models/ModelManager.hpp"
#include "OpenGL/OpenGLWindow.hpp"  // Just for the Ca*EventT classes...
#include "String.hpp"
#include "TypeSys.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#include <cassert>
#include <cstring>

#if defined(_WIN32) && defined (_MSC_VER)
    #if (_MSC_VER<1300)
        #define for if (false) ; else for

        // Turn off warning 4786: "Bezeichner wurde auf '255' Zeichen in den Debug-Informationen reduziert."
        #pragma warning(disable:4786)
    #endif
#endif


using namespace cf::GuiSys;


GuiResourcesT::GuiResourcesT(ModelManagerT& ModelMan)
    : m_ModelMan(ModelMan)
{
}


GuiResourcesT::~GuiResourcesT()
{
    for (unsigned long FontNr=0; FontNr<m_Fonts.Size(); FontNr++)
        delete m_Fonts[FontNr];
}


cf::TrueTypeFontT* GuiResourcesT::GetFont(const std::string& FontName)
{
    // See if FontName has been loaded successfully before.
    for (unsigned long FontNr=0; FontNr<m_Fonts.Size(); FontNr++)
        if (m_Fonts[FontNr]->GetName()==FontName)
            return m_Fonts[FontNr];

    // See if FontName has been loaded UNsuccessfully before.
 // for (unsigned long FontNr=0; FontNr<m_FontsFailed.Size(); FontNr++)
 //     if (m_FontsFailed[FontNr]==FontName)
 //         return NULL;

    // FontName has never been attempted to be loaded, try now.
    try
    {
        m_Fonts.PushBack(new cf::TrueTypeFontT(FontName));
        return m_Fonts[m_Fonts.Size()-1];
    }
    catch (const TextParserT::ParseError&) { }

    Console->Warning(std::string("Failed to load font \"")+FontName+"\".\n");
 // FontsFailed.PushBack(FontName);
    return m_Fonts.Size()>0 ? m_Fonts[0] : NULL;
}


const CafuModelT* GuiResourcesT::GetModel(const std::string& FileName, std::string& ErrorMsg)
{
    return m_ModelMan.GetModel(FileName, &ErrorMsg);
}


GuiImplT::InitErrorT::InitErrorT(const std::string& Message)
    : std::runtime_error(Message)
{
}


GuiImplT::GuiImplT(GuiResourcesT& GuiRes, const std::string& GuiScriptName, bool IsInlineCode)
    : ScriptName(IsInlineCode ? "" : GuiScriptName),
      m_ScriptState(),
      ScriptInitResult(""),
      m_MaterialMan(),
      m_GuiDefaultRM(NULL),
      m_GuiPointerRM(NULL),
      m_GuiFinishZRM(NULL),
      m_GuiResources(GuiRes),
      RootWindow(NULL),
      FocusWindow(NULL),
      MouseOverWindow(NULL),
      IsActive(true),
      IsInteractive(true),
      IsFullCover(false),
      MousePosX(VIRTUAL_SCREEN_SIZE_X/2.0f),   // 320.0f
      MousePosY(VIRTUAL_SCREEN_SIZE_Y/2.0f),   // 240.0f
      MouseIsShown(true)
{
    if (!IsInlineCode)
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

    m_GuiDefaultRM=MatSys::Renderer->RegisterMaterial(m_MaterialMan.GetMaterial("Gui/Default"));
    m_GuiPointerRM=MatSys::Renderer->RegisterMaterial(m_MaterialMan.GetMaterial("Gui/Cursors/Pointer"));
    m_GuiFinishZRM=MatSys::Renderer->RegisterMaterial(m_MaterialMan.GetMaterial("Gui/FinishZ"));


    lua_State* LuaState = m_ScriptState.GetLuaState();

    // Load the console library. (Adds a global table with name "Console" to the LuaState with the functions of the ConsoleI interface.)
    cf::Console_RegisterLua(LuaState);

    // Load the "ci" (console interpreter) library. (Adds a global table with name "ci" to the LuaState with (some of) the functions of the ConsoleInterpreterI interface.)
    ConsoleInterpreterI::RegisterLua(LuaState);

    // Adds a global (meta-)table with methods for cf::GuiSys::GuiTs to the LuaState, to be used as metatable for userdata of type cf::GuiSys::GuiT.
    GuiImplT::RegisterLua(LuaState);

    // For each (window-)class that the TypeInfoMan knows about, add a (meta-)table to the registry of the LuaState.
    // The (meta-)table holds the Lua methods that the respective class implements in C++,
    // and is to be used as metatable for instances of this class.
    m_ScriptState.Init(GetWindowTIM());


    // Add a global variable with name "gui" to the Lua state. "gui" is a table that scripts can use to call GUI methods.
    {
        assert(lua_gettop(LuaState)==0);

        // Stack indices of the table and userdata that we create.
        const int USERDATA_INDEX=2;
        const int TABLE_INDEX   =1;

        // Create a new table T, which is pushed on the stack and thus at stack index TABLE_INDEX.
        lua_newtable(LuaState);

        // Create a new user datum UD, which is pushed on the stack and thus at stack index USERDATA_INDEX.
        GuiImplT** UserData=(GuiImplT**)lua_newuserdata(LuaState, sizeof(GuiImplT*));

        // Initialize the memory allocated by the lua_newuserdata() function.
        *UserData=this;

        // T["__userdata_cf"] = UD
        lua_pushvalue(LuaState, USERDATA_INDEX);    // Duplicate the userdata on top of the stack (as the argument for lua_setfield()).
        lua_setfield(LuaState, TABLE_INDEX, "__userdata_cf");

        // Get the table with name (key) "cf::GuiSys::GuiT" from the registry,
        // and set it as metatable of the newly created table.
        luaL_getmetatable(LuaState, "cf::GuiSys::GuiT");
        lua_setmetatable(LuaState, TABLE_INDEX);

        // Get the table with name (key) "cf::GuiSys::GuiT" from the registry,
        // and set it as metatable of the newly created user data (for user data type safety, see PiL2, chapter 28.2).
        luaL_getmetatable(LuaState, "cf::GuiSys::GuiT");
        lua_setmetatable(LuaState, USERDATA_INDEX);

        // Remove UD from the stack, so that only the new table T is left on top of the stack.
        // Then add it as a global variable whose name is "gui".
        // As lua_setglobal() pops the table from the stack, the stack is left empty.
        lua_pop(LuaState, 1);
        lua_setglobal(LuaState, "gui");
        // Could instead do   lua_setfield(LuaState, LUA_REGISTRYINDEX, "gui");   as well,
        // so that script methods like "new()" and "thread()" could also be called without the "gui:" prefix.
    }

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);


    // Load the user script!
    const int LoadResult=IsInlineCode ? luaL_loadstring(LuaState, GuiScriptName.c_str())
                                      : luaL_loadfile  (LuaState, GuiScriptName.c_str());

    if (LoadResult!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
    {
        Console->Warning(std::string("Lua script \"")+GuiScriptName+"\" could not be loaded\n");
        ScriptInitResult=lua_tostring(LuaState, -1);
        Console->Print("("+ScriptInitResult+").\n");
        lua_pop(LuaState, 1);
    }

    // Make sure that everyone dealt properly with the Lua stack so far.
    assert(lua_gettop(LuaState)==0);


    if (RootWindow==NULL)
    {
        Console->Warning("No root window set for GUI \""+GuiScriptName+"\"!\n");

        // Note that just running some Lua code like "gui:SetRootWindow(gui:new('WindowT'));"
        // here in order to save the situation is not as easy as it seems, because running
        // this code is not guaranteed to be fail-safe and thus not guaranteed to fix the
        // problem. That is, there might still be a case left where we might want to throw.
        lua_close(LuaState);
        MatSys::Renderer->FreeMaterial(m_GuiDefaultRM);
        MatSys::Renderer->FreeMaterial(m_GuiPointerRM);
        MatSys::Renderer->FreeMaterial(m_GuiFinishZRM);
        throw InitErrorT("No root window set. Probable cause:\n"+ScriptInitResult);
    }


    // Finally call the Lua OnInit() and OnInit2() methods of each window.
    ArrayT<WindowT*> AllChildren;

    AllChildren.PushBack(RootWindow.GetRaw());
    RootWindow->GetChildren(AllChildren, true);

    for (unsigned long ChildNr=0; ChildNr<AllChildren.Size(); ChildNr++)
    {
        // The OnInit() methods are automatically written by the Cafu GUI editor (*_init.cgui files).
        AllChildren[ChildNr]->CallLuaMethod("OnInit");
    }

    for (unsigned long ChildNr=0; ChildNr<AllChildren.Size(); ChildNr++)
    {
        // The OnInit2() methods contain custom, hand-written code by the user (*_main.cgui files).
        AllChildren[ChildNr]->CallLuaMethod("OnInit2");
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


void GuiImplT::Activate(bool doActivate)
{
    IsActive=doActivate;

    // Call the OnActivate() or OnDeactivate() methods of all windows.
    ArrayT<WindowT*> AllChildren;

    AllChildren.PushBack(RootWindow.GetRaw());
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
    WindowT* Win=RootWindow->Find(MousePosX, MousePosY);

    if (Win!=MouseOverWindow.GetRaw())
    {
        if (MouseOverWindow!=NULL) MouseOverWindow->CallLuaMethod("OnMouseLeave");
        MouseOverWindow=Win;
        if (MouseOverWindow!=NULL) MouseOverWindow->CallLuaMethod("OnMouseEnter");
    }
}


void GuiImplT::SetShowMouse(bool ShowMouse_)
{
    MouseIsShown=ShowMouse_;
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
        const float b=(EntityName=="") ? 20.0f : 40.0f;     // The mouse cursor size (double for world GUIs).

        Mesh.Vertices[0].SetOrigin(MousePosX,   MousePosY  );
        Mesh.Vertices[1].SetOrigin(MousePosX+b, MousePosY  );
        Mesh.Vertices[2].SetOrigin(MousePosX+b, MousePosY+b);
        Mesh.Vertices[3].SetOrigin(MousePosX,   MousePosY+b);

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
    for (WindowT* Win=FocusWindow.GetRaw(); Win!=NULL; Win=Win->GetParent())
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
            if (!MEWasProcessed && ME.Type==CaMouseEventT::CM_BUTTON0 && ButtonDown && MouseOverWindow!=FocusWindow.GetRaw())
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

    float AbsWinPosX;
    float AbsWinPosY;

    MouseOverWindow->GetAbsolutePos(AbsWinPosX, AbsWinPosY);

    return MouseOverWindow->OnInputEvent(ME, MousePosX-AbsWinPosX, MousePosY-AbsWinPosY);
}


void GuiImplT::DistributeClockTickEvents(float t)
{
    if (GetIsActive())  // Inconsistent and inconsequent, but saves performance: Only distribute clock tick events when this GUI is active.
    {
        ArrayT<WindowT*> AllChildren;

        AllChildren.PushBack(RootWindow.GetRaw());
        RootWindow->GetChildren(AllChildren, true);

        for (unsigned long ChildNr=0; ChildNr<AllChildren.Size(); ChildNr++)
        {
            AllChildren[ChildNr]->OnClockTickEvent(t);
            AllChildren[ChildNr]->CallLuaMethod("OnFrame");
        }
    }

    // Run the pending coroutines always, even if this GUI is currently not active.
    m_ScriptState.RunPendingCoroutines(t);
}


bool GuiImplT::CallLuaFunc(const char* FuncName, const char* Signature, ...)
{
    lua_State* LuaState = m_ScriptState.GetLuaState();

    // Note that when re-entrancy occurs, we do usually NOT have an empty stack here!
    // That is, when we first call a Lua function the stack is empty, but when the called Lua function
    // in turn calls back into our C++ code (e.g. a console function), and the C++ code in turn gets here,
    // we have a case of re-entrancy and the stack is not empty!
    // That is, the assert() statement in the next line does not generally hold.
    // assert(lua_gettop(LuaState)==0);

    // Get the desired global function.
    lua_getglobal(LuaState, FuncName);

    if (!lua_isfunction(LuaState, -1))
    {
        // If we get here, this usually means that the value at -1 is just nil, i.e. the
        // function that we would like to call was just not defined in the Lua script.
        lua_pop(LuaState, 1);   // Pop whatever is not a function.
        return false;
    }

    va_list vl;

    va_start(vl, Signature);
    const bool Result=m_ScriptState.StartNewCoroutine(0, Signature, vl, std::string("global function ")+FuncName+"()");
    va_end(vl);

    return Result;
}


bool GuiImplT::CallLuaMethod(WindowPtrT Window, const char* MethodName, const char* Signature, ...)
{
    va_list vl;

    va_start(vl, Signature);
    const bool Result=Window->CallLuaMethod(MethodName, Signature, vl);
    va_end(vl);

    return Result;
}


void GuiImplT::SetEntityInfo(const char* EntityName_, void* /*EntityInstancePtr_*/)
{
    EntityName=EntityName_;
 // EntityInstancePtr=EntityInstancePtr_;
}


void GuiImplT::RegisterScriptLib(const char* LibName, const luaL_Reg Functions[])
{
    lua_State* LuaState = m_ScriptState.GetLuaState();

    luaL_register(LuaState, LibName, Functions);
    lua_pop(LuaState, 1);   // Remove the LibName table from the stack (it was left there by the luaL_register() function).
}


/**********************************************/
/*** Impementation of Lua binding functions ***/
/**********************************************/

static GuiImplT* CheckParams(lua_State* LuaState)
{
    luaL_argcheck(LuaState, lua_istable(LuaState, 1), 1, "Expected a table that represents a GUI.");
    lua_getfield(LuaState, 1, "__userdata_cf");

    GuiImplT** UserData=(GuiImplT**)luaL_checkudata(LuaState, -1, "cf::GuiSys::GuiT"); if (UserData==NULL) luaL_error(LuaState, "NULL userdata in GUI table.");
    GuiImplT*  Gui     =(*UserData);

    // Pop the userdata from the stack again. Not necessary though as it doesn't hurt there.
    // lua_pop(LuaState, 1);
    return Gui;
}


int GuiImplT::Activate(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    // I also want to treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(LuaState, 2)) Gui->IsActive=lua_tointeger(LuaState, 2)!=0;
                              else Gui->IsActive=lua_toboolean(LuaState, 2)!=0;

    return 0;
}


int GuiImplT::Close(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    Gui->IsActive=false;
    return 0;
}


int GuiImplT::SetInteractive(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    // I also want to treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(LuaState, 2)) Gui->IsInteractive=lua_tointeger(LuaState, 2)!=0;
                              else Gui->IsInteractive=lua_toboolean(LuaState, 2)!=0;

    return 0;
}


int GuiImplT::SetFullCover(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    // I also want to treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(LuaState, 2)) Gui->IsFullCover=lua_tointeger(LuaState, 2)!=0;
                              else Gui->IsFullCover=lua_toboolean(LuaState, 2)!=0;

    return 0;
}


int GuiImplT::SetMousePos(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    Gui->SetMousePos(float(lua_tonumber(LuaState, 2)),
                     float(lua_tonumber(LuaState, 3)));

    return 0;
}


int GuiImplT::SetMouseMat(lua_State* LuaState)
{
    // GuiImplT* Gui=CheckParams(LuaState);

    return 0;
}


int GuiImplT::SetMouseIsShown(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    // I also want to treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(LuaState, 2)) Gui->MouseIsShown=lua_tointeger(LuaState, 2)!=0;
                              else Gui->MouseIsShown=lua_toboolean(LuaState, 2)!=0;

    return 0;
}


int GuiImplT::SetFocus(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    if (lua_isstring(LuaState, 2))
    {
        Gui->FocusWindow=Gui->RootWindow->Find(lua_tostring(LuaState, 2));
    }
    else if (lua_istable(LuaState, 2))
    {
        WindowT* Win=(WindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 2, WindowT::TypeInfo);

        Gui->FocusWindow=Win;
    }

    // Note that we intentionally did *not* call the Lua OnFocusLose() or OnFocusGain() scripts,
    // as is done when the focus changes due to a mouse click:
    // the Lua code that calls this method should instead call the desired OnFocus*() script functions itself.
    // This behaviour must be explicitly stated in the user documentation!
    return 0;
}


int GuiImplT::HasValidEntity(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    lua_pushboolean(LuaState, /*Gui->EntityInstancePtr!=NULL &&*/ Gui->EntityName!="");
    return 1;
}


int GuiImplT::GetEntityName(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    lua_pushstring(LuaState, Gui->EntityName.c_str());
    return 1;
}


int GuiImplT::SetRootWindow(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);
    WindowT*  Win=(WindowT*)cf::GuiSys::GuiImplT::GetCheckedObjectParam(LuaState, 2, WindowT::TypeInfo);

    // Note that Gui->RootWindow is a WindowPtrT that makes sure that the Win instance gets
    // properly anchored in the Lua state in order to prevent premature garbage collection.
    Gui->RootWindow=Win;
    return 0;
}


int GuiImplT::CreateNewWindow(lua_State* LuaState)
{
    GuiImplT*                     Gui=CheckParams(LuaState);
    const char*                   TypeName=luaL_checkstring(LuaState, 2);
    const char*                   WinName=lua_tostring(LuaState, 3);    // Passing a window name is optional.
    const cf::TypeSys::TypeInfoT* TI=GetWindowTIM().FindTypeInfoByName(TypeName);

    if (!TI) return luaL_argerror(LuaState, 2, (std::string("unknown window class \"")+TypeName+"\"").c_str());

    WindowT* Win=static_cast<WindowT*>(TI->CreateInstance(WindowCreateParamsT(*Gui)));  // Actually create the window instance.

    // Console->DevPrint(cf::va("Creating window %p.\n", Win));
    assert(Win->GetType()==TI);
    assert(strcmp(TI->ClassName, TypeName)==0);

    if (WinName) Win->Name=WinName;


    // ANALOGY: This code is ANALOGOUS TO that in Games/DeathMatch/Code/ScriptState.cpp.
    // Now do the actual Lua work: add the new table that represents the window.
    // Stack indices of the table and userdata that we create in this loop.
    const int USERDATA_INDEX=lua_gettop(LuaState)+2;
    const int TABLE_INDEX   =lua_gettop(LuaState)+1;

    // Create a new table T, which is pushed on the stack and thus at stack index TABLE_INDEX.
    lua_newtable(LuaState);

    // Create a new user datum UD, which is pushed on the stack and thus at stack index USERDATA_INDEX.
    WindowT** UserData=(WindowT**)lua_newuserdata(LuaState, sizeof(WindowT*));

    // Initialize the memory allocated by the lua_newuserdata() function.
    *UserData=Win;

    // T["__userdata_cf"] = UD
    lua_pushvalue(LuaState, USERDATA_INDEX);    // Duplicate the userdata on top of the stack (as the argument for lua_setfield()).
    lua_setfield(LuaState, TABLE_INDEX, "__userdata_cf");

    // Get the table with name (key) Win->GetType()->ClassName (== TI->ClassName == TypeName) from the registry,
    // and set it as metatable of the newly created table.
    // This is the crucial step that establishes the main functionality of our new table.
    luaL_getmetatable(LuaState, TypeName);
    lua_setmetatable(LuaState, TABLE_INDEX);

    // Get the table with name (key) Win->GetType()->ClassName (== TI->ClassName == TypeName) from the registry,
    // and set it as metatable of the newly created userdata item.
    // This is important for userdata type safety (see PiL2, chapter 28.2) and to have automatic garbage collection work
    // (contrary to the text in the "Game Programming Gems 6" book, chapter 4.2, a __gc method in the metatable
    //  is only called for full userdata, see my email to the Lua mailing list on 2008-Apr-01 for more details).
    luaL_getmetatable(LuaState, TypeName);
    lua_setmetatable(LuaState, USERDATA_INDEX);

    // Remove UD from the stack, so that only the new table T is left on top of the stack.
    lua_pop(LuaState, 1);


    // There remains one final chore:
    // Later, when given a pointer to a WindowT, we want to be able to find the table T by that pointer.
    // For example, this occurs in WindowT::CallLuaMethod() with the "this" pointer.
    // We therefore proceed as described in the PiL2, chapter 28.5 ("Light Userdata"):
    // Create a table where the indices are light userdata with the WindowT pointers,
    // and the values are the T tables and have the "weak" property.
    // (Note that we do not have to remove this entry explicitly ever again later, because it expires automatically
    //  when Lua garbage collects T. See the PiL2 book, chapter 17 ("Weak Tables") for more details.)
    lua_getfield(LuaState, LUA_REGISTRYINDEX, "__windows_list_cf");

    if (!lua_istable(LuaState, -1))
    {
        lua_pop(LuaState, 1);           // Remove whatever was not a table.
        lua_newtable(LuaState);         // Push a new table LIST instead.

        lua_newtable(LuaState);         // Push another new table. This will become the metatable for LIST.
        lua_pushstring(LuaState, "v");
        lua_setfield(LuaState, -2, "__mode");
        lua_setmetatable(LuaState, -2); // Set the table   { __mode="v" }   as the metatable of LIST.

        lua_pushvalue(LuaState, -1);    // Duplicate LIST for setting it as a value in the registry.
        lua_setfield(LuaState, LUA_REGISTRYINDEX, "__windows_list_cf");
    }

    lua_pushlightuserdata(LuaState, Win);
    lua_pushvalue(LuaState, TABLE_INDEX);
    lua_rawset(LuaState, -3);       // __windows_list_cf[Win] = T
    lua_pop(LuaState, 1);           // Remove __windows_list_cf from the stack top again.


    // Finally done: return the new table T.
    assert(lua_gettop(LuaState)==TABLE_INDEX);
    return 1;
}


int GuiImplT::FindWindow(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);
    WindowT*  Find=(WindowT*)luaL_checklong(LuaState, 2);

    ArrayT<WindowT*> AllChildren;

    AllChildren.PushBack(Gui->RootWindow.GetRaw());
    Gui->RootWindow->GetChildren(AllChildren, true);

    for (unsigned long ChildNr=0; ChildNr<AllChildren.Size(); ChildNr++)
        if (AllChildren[ChildNr]==Find)
        {
            // This is the same as Find->PushAlterEgo(). I repeat the implementation here
            // so that I don't have to move PushAlterEgo() from private to public visibility.
            lua_getfield(LuaState, LUA_REGISTRYINDEX, "__windows_list_cf");
            lua_pushlightuserdata(LuaState, Find);
            lua_rawget(LuaState, -2);
            lua_remove(LuaState, -2);
            return 1;
        }

    return 0;
}


int GuiImplT::toString(lua_State* LuaState)
{
    GuiImplT* Gui=CheckParams(LuaState);

    if (Gui->ScriptName=="") lua_pushfstring(LuaState, "A programmatically generated GUI.");
                        else lua_pushfstring(LuaState, "A gui generated from the script \"%s\".", Gui->ScriptName.c_str());

    return 1;
}


void GuiImplT::RegisterLua(lua_State* LuaState)
{
    // Create a new table T and add it into the registry table with "cf::GuiSys::GuiT" as the key and T as the value.
    // This also leaves T on top of the stack. See PiL2 chapter 28.2 for more details.
    luaL_newmetatable(LuaState, "cf::GuiSys::GuiT");

    // See PiL2 chapter 28.3 for a great explanation on what is going on here.
    // Essentially, we set T.__index = T (the luaL_newmetatable() function left T on the top of the stack).
    lua_pushvalue(LuaState, -1);                // Pushes/duplicates the new table T on the stack.
    lua_setfield(LuaState, -2, "__index");      // T.__index = T;

    static const luaL_reg GuiMethods[]=
    {
        { "activate",       Activate },
        { "close",          Close },
        { "setInteractive", SetInteractive },
        { "setFullCover",   SetFullCover },
        { "setMousePos",    SetMousePos },
        { "setMouseMat",    SetMouseMat },
        { "showMouse",      SetMouseIsShown },
        { "setFocus",       SetFocus },
        { "hasValidEntity", HasValidEntity },
        { "getEntityName",  GetEntityName },
        { "SetRootWindow",  SetRootWindow },
        { "new",            CreateNewWindow },
        { "FindWindow",     FindWindow },
        { "__tostring",     toString },
        { NULL, NULL }
    };

    // Now insert the functions listed in GuiMethods into T (the table on top of the stack).
    luaL_register(LuaState, NULL, GuiMethods);

    // Clear the stack.
    lua_settop(LuaState, 0);
}


// ANALOGY: This code is ANALOGOUS TO that in Games/DeathMatch/Code/ScriptState.cpp.
// There is no compelling reason to keep this static function inside this .cpp file or as a member of the GuiImplT class,
// except for the fact that it relies on knowledge that is only found here: The way how metatables are metatables of each
// other when inheritance occurs, the private "__userdata_cf" string, etc.
/*static*/ void* GuiImplT::GetCheckedObjectParam(lua_State* LuaState, int StackIndex, const cf::TypeSys::TypeInfoT& TypeInfo)
{
    // First make sure that the table that represents the window itself is at StackIndex.
    luaL_argcheck(LuaState, lua_istable(LuaState, StackIndex), StackIndex, "Expected a table that represents a window." /*of type TypeInfo.ClassName*/);

    // Put the contents of the "__userdata_cf" field on top of the stack (other values may be between it and the table at position StackIndex).
    lua_getfield(LuaState, StackIndex, "__userdata_cf");

#if 1
    // This approach takes inheritance properly into account by "manually traversing up the inheriance hierarchy".
    // See the "Game Programming Gems 6" book, page 353 for the inspiration for this code.

    // Put the metatable of the desired type on top of the stack.
    luaL_getmetatable(LuaState, TypeInfo.ClassName);

    // Put the metatable for the given userdata on top of the stack (it may belong to a derived class).
    if (!lua_getmetatable(LuaState, -2)) lua_pushnil(LuaState);     // Don't have it push nothing in case of failure.

    while (lua_istable(LuaState, -1))
    {
        if (lua_rawequal(LuaState, -1, -2))
        {
            void** UserData=(void**)lua_touserdata(LuaState, -3); if (UserData==NULL) luaL_error(LuaState, "NULL userdata in window table.");
            void*  Window  =(*UserData);

            // Pop the two matching metatables and the userdata.
            lua_pop(LuaState, 3);
            return Window;
        }

        // Replace the metatable on top of the stack with its metatable (i.e. "the metatable of the metatable").
        if (!lua_getmetatable(LuaState, -1)) lua_pushnil(LuaState);     // Don't have it push nothing in case of failure.
        lua_replace(LuaState, -2);
    }

    luaL_typerror(LuaState, StackIndex, TypeInfo.ClassName);
    return NULL;
#else
    // This approach is too simplistic and thus doesn't work when inheritance is used.
    void** UserData=(void**)luaL_checkudata(LuaState, -1, TypeInfo.ClassName); if (UserData==NULL) luaL_error(LuaState, "NULL userdata in window table.");
    void*  Window  =(*UserData);

    // Pop the userdata from the stack again. Not necessary though as it doesn't hurt there.
    // lua_pop(LuaState, 1);
    return Window;
#endif
}
