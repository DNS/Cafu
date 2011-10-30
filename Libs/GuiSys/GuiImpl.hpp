/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#ifndef _CF_GUISYS_GUI_IMPL_HPP_
#define _CF_GUISYS_GUI_IMPL_HPP_

#include "Gui.hpp"
#include "Coroutines.hpp"
#include "MaterialSystem/MaterialManagerImpl.hpp"

#include <cstdarg>
#include <stdexcept>


namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { class TrueTypeFontT; }
namespace MatSys { class RenderMaterialT; }
class CafuModelT;
class ModelManagerT;
struct lua_State;


namespace cf
{
    namespace GuiSys
    {
        /// This class manages and provides resources (fonts and models) for GuiImplT instances.
        /// One GuiResourcesT can be commonly used for several GuiImplT instances at once.
        class GuiResourcesT
        {
            public:

            /// The constructor.
            GuiResourcesT(ModelManagerT& ModelMan);

            /// The destructor.
            ~GuiResourcesT();

            /// Returns (a pointer to) a font instance for the given filename.
            /// The returned font instance is possibly shared with other users, and must not be deleted.
            /// @param FontName   The name of the font to return.
            /// @returns A pointer to the specified font, or NULL if there was an error (e.g. the font could not be loaded).
            TrueTypeFontT* GetFont(const std::string& FontName);

            /// Returns (a pointer to) a model instance for the given filename.
            /// @see ModelManagerT::GetModel() for more details.
            const CafuModelT* GetModel(const std::string& FileName, std::string& ErrorMsg);


            private:

            GuiResourcesT(const GuiResourcesT&);        ///< Use of the Copy Constructor    is not allowed.
            void operator = (const GuiResourcesT&);     ///< Use of the Assignment Operator is not allowed.

            ArrayT<TrueTypeFontT*> m_Fonts;     ///< The fonts that are used within the GUIs.
            ModelManagerT&         m_ModelMan;  ///< The model manager from which any models that occur in the GUIs are aquired.
        };


        /// This class implements the GuiI interface.
        /// TODO / FIXME:
        /// - Should we use different metatables for the table representation of a window and its userdata item (contained at table index __userdata_cf)?
        /// - Should we use for the __index value of a metatable not the metatable itself (so that __gc is not accessible from the script anymore)?
        ///   (Search the Lua users mailing list for "__gc" in this regard.)
        /// - How does this affect GuiImplT::GetCheckedObjectParam()?
        /// - Employ the \emph{environment} of userdata for our private data (e.g. the __children_cf and __parent_cf fields)!
        class GuiImplT : public GuiI
        {
            public:

            class InitErrorT;


            /// Constructor for creating a window hierarchy (=="a GUI") from the GUI script file GuiScriptName.
            /// @param GuiRes          The provider for resources (fonts and models) that are used in this GUI.
            /// @param GuiScriptName   The file name of the GUI script to load or inline script code (depending on IsInlineCode).
            /// @param IsInlineCode    Whether GuiScriptName is inline script code or a filename.
            /// @throws an InitErrorT object on problems initializing the GUI.
            GuiImplT(GuiResourcesT& GuiRes, const std::string& GuiScriptName, bool IsInlineCode=false);

            /// The destructor.
            ~GuiImplT();

            /// Returns the string with the result of loading and running the GUI script.
            /// Note that errors reported here are not necessarily fatal: the GUI may be usable (at least partially) anyway.
            /// @returns the empty string when there have been no errors, or the error message otherwise.
            const std::string& GetScriptInitResult() const { return ScriptInitResult; }

            /// Returns the material manager instance of this GUI.
            const MaterialManagerImplT& GetMaterialManager() const { return m_MaterialMan; }

            /// Returns the default RenderMaterialT that should be used for borders and backgrounds if no other material is specified for that window.
            MatSys::RenderMaterialT* GetDefaultRM() const;

            /// Returns the (default) RenderMaterialT for the mouse pointer.
            MatSys::RenderMaterialT* GetPointerRM() const;

            /// Returns the resource provider for fonts and models that are used in this GUI.
            GuiResourcesT& GetGuiResources() const { return m_GuiResources; }


            // Implement all the (pure) virtual methods of the GuiI interface.
            const std::string& GetScriptName() const;
            WindowPtrT GetRootWindow() const { return RootWindow; }
            WindowPtrT GetFocusWindow() const { return FocusWindow; }
            void Activate(bool doActivate=true);
            bool GetIsActive() const { return IsActive; }
            void SetInteractive(bool IsInteractive_=true);
            bool GetIsInteractive() const { return IsInteractive; }
            bool GetIsFullCover() const { return IsFullCover; }
            void GetMousePos(float& MousePosX_, float& MousePosY_) const;
            void SetMousePos(float MousePosX_, float MousePosY_);
            void SetShowMouse(bool ShowMouse_);
            bool IsMouseShown() const { return MouseIsShown; }
            void Render() const;
            bool ProcessDeviceEvent(const CaKeyboardEventT& KE);
            bool ProcessDeviceEvent(const CaMouseEventT& ME);
            void DistributeClockTickEvents(float t);
            bool CallLuaFunc(const char* FuncName, const char* Signature="", ...);
            bool CallLuaMethod(WindowPtrT Window, const char* MethodName, const char* Signature="", ...);
            void SetEntityInfo(const char* EntityName_, void* EntityInstancePtr_);
            void RegisterScriptLib(const char* LibName, const luaL_Reg Functions[]);


            /// Adds a new global variable of type (meta-)table and name "cf::GuiSys::GuiT" to the Lua state LuaState,
            /// containing functions (or rather "methods") that can be called on userdata objects of type cf::GuiSys::GuiT.
            /// This is very analogous to how normal C-code modules are registered with Lua, except for
            /// the fact that this table is intended to be set as metatable for userdata objects of type cf::GuiSys::GuiT.
            /// For more details, see the implementation of this function and the PiL2 book, chapter 28.1 to 28.3.
            /// @param LuaState DOCTODO
            static void RegisterLua(lua_State* LuaState);

            /// Checks if the value at the given stack index is a window of type TypeInfo, and returns the userdata which is a pointer to the instance.
            static void* GetCheckedObjectParam(lua_State* LuaState, int StackIndex, const cf::TypeSys::TypeInfoT& TypeInfo);


            private:

            friend class WindowT;
            friend class WindowPtrT;


            bool StartNewCoroutine(int NumExtraArgs, const char* Signature, va_list vl, const std::string& DbgName);
            void RunPendingCoroutines(float FrameTime);

            GuiImplT(const GuiImplT&);          ///< Use of the Copy Constructor    is not allowed.
            void operator = (const GuiImplT&);  ///< Use of the Assignment Operator is not allowed.


            std::string              ScriptName;        ///< The name of the *.cgui file that contains this GUI's script.
            lua_State*               LuaState;          ///< The Lua program state of this GUI.
            CoroutineManT            CoroutineMan;      ///< This class manages (keeps track of) the pending Lua coroutines.
            std::string              ScriptInitResult;  ///< The result of loading and running the script. "" if there have been no errors, the error message otherwise.
            MaterialManagerImplT     m_MaterialMan;     ///< The material manager for the materials that are used in this GUI.
            MatSys::RenderMaterialT* m_GuiDefaultRM;    ///< Used for the window borders and the backgrounds if no other material is specified.
            MatSys::RenderMaterialT* m_GuiPointerRM;    ///< Used for the mouse pointer.
            GuiResourcesT&           m_GuiResources;    ///< The provider for resources (fonts and models) that are used in this GUI.

            WindowPtrT               RootWindow;        ///< The root window of the window hierarchy that forms this GUI.
            WindowPtrT               FocusWindow;       ///< The window in the hierachy that currently has the (keyboard) input focus.
            WindowPtrT               MouseOverWindow;   ///< The window that the mouse is currently hovering over.

            bool                     IsActive;          ///< Whether this GUI is active or not. This is of importance mainly for the GuiMan, which doesn't send us events and doesn't draw us if we're not active.
            bool                     IsInteractive;     ///< Whether this GUI is interactive (reacts to device events) or not. This is of importance mainly for the GuiMan, which doesn't send us device events if we are not interactive, and sends device events only to the top-most interactive GUI.
            bool                     IsFullCover;       ///< Whether this GUI is fullscreen and fully opaque, i.e. whether this GUI covers everything under it. If true, the GuiSys saves the rendering of the GUIs "below" this one. This can improve the GUI performance significantly if e.g. the player is at a point in the game where the world rendering FPS is low.
            float                    MousePosX;         ///< The x-coordinate of the position of the mouse cursor.
            float                    MousePosY;         ///< The y-coordinate of the position of the mouse cursor.
            bool                     MouseIsShown;      ///< Whether the mouse cursor is shown. Non-interactive GUIs normally don't show a cursor.

            std::string              EntityName;        ///< If this is a 3D world GUI, this is the name of its parent entity.
         // void*                    EntityInstancePtr; ///< If this is a 3D world GUI, this is the pointer to the instance of its parent entity.

            // Gui variables (general purpose)... (Maus-unabhängig, z.B. aktuelle Lift-Position............. übers Netzwerk sync'en!!)
            // ...


            // Methods called from Lua scripts on cf::GuiSys::GuiTs.
            static int Activate(lua_State* LuaState);           ///< Sets the IsActive flag of this GUI.
            static int Close(lua_State* LuaState);              ///< Same as calling "gui:activate(false);".
            static int SetInteractive(lua_State* LuaState);     ///< Sets the IsInteractive flag of this GUI.
            static int SetFullCover(lua_State* LuaState);       ///< Sets the IsFullCover flag of this GUI.
            static int SetMousePos(lua_State* LuaState);        ///< Sets the position of the mouse cursor.
            static int SetMouseMat(lua_State* LuaState);        ///< Sets the material that is used to render the mouse cursor.
            static int SetMouseIsShown(lua_State* LuaState);    ///< Determines whether the mouse cursor is shown at all.
            static int SetFocus(lua_State* LuaState);           ///< Sets the keyboard input focus to the given window. Does *not* call the Lua OnFocusLose() or OnFocusGain() scripts!
            static int HasValidEntity(lua_State* LuaState);     ///< Returns true if this GUI is a 3D world GUI (an implicit requirement), it has been assigned a valid parent entity and the parent entity name is non-empty. Returns false otherwise.
            static int GetEntityName(lua_State* LuaState);      ///< Returns the non-empty name of the parent entity if HasValidEntity() returns true, an empty string otherwise.
            static int SetRootWindow(lua_State* LuaState);      ///< Sets the root window for this GUI.
            static int CreateNewWindow(lua_State* LuaState);    ///< Creates and returns a new window.
            static int FindWindow(lua_State* LuaState);         ///< Finds and returns a window by pointer value. Useful for debugging when an error message referred to a window by pointer value.
            static int RegisterThread(lua_State* LuaState);     ///< Registers the given Lua function as a new thread.
            static int toString(lua_State* LuaState);           ///< Returns a string representation of this GUI.
        };
    }
}


/// A class that is thrown on GUI initialization errors.
class cf::GuiSys::GuiImplT::InitErrorT : public std::runtime_error
{
    public:

    InitErrorT(const std::string& Message);
};

#endif
