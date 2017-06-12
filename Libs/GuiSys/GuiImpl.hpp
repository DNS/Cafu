/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUISYS_GUI_HPP_INCLUDED
#define CAFU_GUISYS_GUI_HPP_INCLUDED

#include "MaterialSystem/MaterialManagerImpl.hpp"
#include "Templates/Pointer.hpp"
#include "TypeSys.hpp"

#include <stdexcept>


namespace cf { namespace Network { class InStreamT; } }
namespace cf { namespace Network { class OutStreamT; } }
namespace cf { class UniScriptStateT; }
namespace MatSys { class RenderMaterialT; }
struct CaKeyboardEventT;
struct CaMouseEventT;
struct lua_State;


namespace cf
{
    namespace GuiSys
    {
        /// The TypeInfoTs of all GuiImplT-derived classes must register with this TypeInfoManT instance.
        cf::TypeSys::TypeInfoManT& GetGuiTIM();


        class GuiResourcesT;
        class WindowT;

        /// Note that it is very difficult to change these constants later, because then all GUI scripts
        /// in the world had to be changed too (and in a non-trivial way)!
        const float VIRTUAL_SCREEN_SIZE_X=640.0f;
        const float VIRTUAL_SCREEN_SIZE_Y=480.0f;


        /// This class implements a Graphical User Interface (GUI).
        class GuiImplT : public RefCountedT
        {
            public:

            class InitErrorT;

            /// Flags for initializing a GUI from a script.
            enum InitFlagsT
            {
                InitFlag_InlineCode  = 1,   ///< Normally, the `GuiScriptName` parameter to the GuiImplT ctor is a filename. If this is set, it is treated as inline script code.
                InitFlag_InGuiEditor = 2    ///< Whether the GUI is instantiated in the GUI Editor. If set, only the static data will be loaded, initial behaviour is *not* run.
            };

            /// Initializes the given script state for use with GuiImplT instances.
            static void InitScriptState(UniScriptStateT& ScriptState);


            /// Constructor for creating a window hierarchy (=="a GUI") from the GUI script file GuiScriptName.
            /// @param ScriptState   The caller will use this GUI with this script state (binds the GUI to it).
            /// @param GuiRes        The provider for resources (fonts and models) that are used in this GUI.
            GuiImplT(UniScriptStateT& ScriptState, GuiResourcesT& GuiRes);

            /// Constructor for creating a window hierarchy (=="a GUI") from the GUI script file GuiScriptName.
            ///
            /// This constructor is *DEPRECATED*. It only exists so that code that uses it can be updated to
            /// the new constructor at a convenient time. It should not be used in any new code.
            ///
            /// @param GuiRes        The provider for resources (fonts and models) that are used in this GUI.
            GuiImplT(GuiResourcesT& GuiRes);

            /// The destructor.
            ~GuiImplT();

            /// A method that is needed when the obsolete, deprecated ctor above is used.
            /// Without this method, it is impossible to break cycles of IntrusivePtrT%s.
            void ObsoleteForceKill();

            /// Loads and runs the given script in order to initialize this GUI instance.
            /// @param ScriptName   The file name of the script to load or inline script code (if InitFlag_InlineCode is set).
            /// @param Flags        A combination of the flags in InitFlagsT.
            /// @throws Throws an InitErrorT object on problems initializing the GUI.
            void LoadScript(const std::string& ScriptName, int Flags = 0);

            /// Returns the material manager instance of this GUI.
            const MaterialManagerImplT& GetMaterialManager() const { return m_MaterialMan; }

            /// Returns the default RenderMaterialT that should be used for borders and backgrounds if no other material is specified for that window.
            MatSys::RenderMaterialT* GetDefaultRM() const;

            /// Returns the (default) RenderMaterialT for the mouse pointer.
            MatSys::RenderMaterialT* GetPointerRM() const;

            /// Returns the resource provider for fonts and models that are used in this GUI.
            GuiResourcesT& GetGuiResources() const { return m_GuiResources; }

            /// Returns the name of the script file of this GUI.
            const std::string& GetScriptName() const;

            /// Returns the script state of this GUI.
            UniScriptStateT& GetScriptState() { return *m_ScriptState; }

            /// Returns the root window of this GUI.
            IntrusivePtrT<WindowT> GetRootWindow() const;

            /// Returns the window in this GUI that has the keyboard input focus.
            IntrusivePtrT<WindowT> GetFocusWindow() const;

            /// Activates or deactivates this GUI.
            void Activate(bool doActivate=true);

            /// Returns whether this GUI is active or not. This is of importance mainly for the GuiMan, which doesn't send us events and doesn't draw us if we're not active.
            bool GetIsActive() const { return IsActive; }

            /// Sets whether this GUI is interactive or not. See GetIsInteractive() for additional information.
            void SetInteractive(bool IsInteractive_=true);

            /// Returns whether this GUI is interactive (reacts to device events) or not. This is of important mainly for the GuiMan, which doesn't send us device events if we are not interactive, and sends device events only to the top-most interactive GUI.
            bool GetIsInteractive() const { return IsInteractive; }

            /// Returns whether this GUI is fullscreen and fully opaque, i.e. whether this GUI covers everything under it. If true, the GuiSys saves the rendering of the GUIs "below" this one. This can improve the GUI performance significantly if e.g. the player is at a point in the game where the world rendering FPS is low.
            bool GetIsFullCover() const { return IsFullCover; }

            /// Returns the position of the mouse cursor.
            void GetMousePos(float& MousePosX_, float& MousePosY_) const;

            /// Sets the position of the mouse cursor.
            void SetMousePos(float MousePosX_, float MousePosY_);

            /// Returns the size of the mouse cursor.
            float GetMouseCursorSize() const { return m_MouseCursorSize; }

            /// Sets the size of the mouse cursor.
            void SetMouseCursorSize(float s) { m_MouseCursorSize = s; }

            /// Sets whether this GUI shows a mouse cursor.
            void SetShowMouse(bool ShowMouse_);

            /// Returns whether this GUI shows a mouse cursor.
            bool IsMouseShown() const { return MouseIsShown; }


            /// See WindowT::Serialize() for details.
            /// This method is only needed to serialize the mouse pos, there is no equivalent in cf::GameSys::WorldT.
            void Serialize(cf::Network::OutStreamT& Stream) const;

            /// See WindowT::Deserialize() for details.
            /// This method is only needed to deserialize the mouse pos, there is no equivalent in cf::GameSys::WorldT.
            void Deserialize(cf::Network::InStreamT& Stream, bool IsIniting);

            /// Renders this GUI.
            /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices:
            /// it's up to the caller to do that.
            /// @param zLayerCoating   Whether a z-layer coating should be applied to the GUI screen when finishing the rendering.
            ///     This is useful whenever the z-ordering of scene elements can be imperfect, e.g. in the Map Editor.
            ///     Generally, 3D world GUIs should use \c true, 2D GUIs should use \c false.
            void Render(bool zLayerCoating=false) const;

            /// Processes a keyboard event by forwarding it to the window that currently has the input focus.
            /// The GuiMan should make the descision to call this method dependend on the result of the GetIsInteractive() method.
            /// @param KE Keyboard event to process.
            /// @returns true if the device has been successfully processed, false otherwise.
            bool ProcessDeviceEvent(const CaKeyboardEventT& KE);

            /// Processes a mouse event by forwarding it to the window that currently has the input focus.
            /// The GuiMan should make the descision to call this method dependend on the result of the GetIsInteractive() method.
            /// @param ME Mouse event to process.
            /// @returns true if the device has been successfully processed, false otherwise.
            bool ProcessDeviceEvent(const CaMouseEventT& ME);

            /// "Creates" a time tick event for each window of the GUI (no matter whether its currently visible (shown) or not)
            /// by calling its OnTimeTickEvent() methods.
            /// @param t   The time in seconds since the last clock-tick.
            void DistributeClockTickEvents(float t);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            GuiImplT(const GuiImplT&);          ///< Use of the Copy Constructor    is not allowed.
            void operator = (const GuiImplT&);  ///< Use of the Assignment Operator is not allowed.

            void Init();    ///< Calls the OnInit() script methods of all windows.


            std::string              ScriptName;        ///< The name of the *.cgui file that contains this GUI's script.
            UniScriptStateT*         m_ScriptState;     ///< The script state of this GUI.
            const bool               m_IsOwnScriptSt;   ///< Are we the owner of the m_ScriptState instance?
            MaterialManagerImplT     m_MaterialMan;     ///< The material manager for the materials that are used in this GUI.
            MatSys::RenderMaterialT* m_GuiDefaultRM;    ///< Used for the window borders and the backgrounds if no other material is specified.
            MatSys::RenderMaterialT* m_GuiPointerRM;    ///< Used for the mouse pointer.
            MatSys::RenderMaterialT* m_GuiFinishZRM;    ///< Used for laying-down z-buffer values after all GUI elements have been rendered.
            GuiResourcesT&           m_GuiResources;    ///< The provider for resources (fonts and models) that are used in this GUI.

            IntrusivePtrT<WindowT>   RootWindow;        ///< The root window of the window hierarchy that forms this GUI.
            IntrusivePtrT<WindowT>   FocusWindow;       ///< The window in the hierachy that currently has the (keyboard) input focus.
            IntrusivePtrT<WindowT>   MouseOverWindow;   ///< The window that the mouse is currently hovering over.

            bool                     m_IsInited;        ///< Has the Init() method already been called?
            bool                     IsActive;          ///< Whether this GUI is active or not. This is of importance mainly for the GuiMan, which doesn't send us events and doesn't draw us if we're not active.
            bool                     IsInteractive;     ///< Whether this GUI is interactive (reacts to device events) or not. This is of importance mainly for the GuiMan, which doesn't send us device events if we are not interactive, and sends device events only to the top-most interactive GUI.
            bool                     IsFullCover;       ///< Whether this GUI is fullscreen and fully opaque, i.e. whether this GUI covers everything under it. If true, the GuiSys saves the rendering of the GUIs "below" this one. This can improve the GUI performance significantly if e.g. the player is at a point in the game where the world rendering FPS is low.
            float                    MousePosX;         ///< The x-coordinate of the position of the mouse cursor.
            float                    MousePosY;         ///< The y-coordinate of the position of the mouse cursor.
            float                    m_MouseCursorSize; ///< The size of the mouse cursor.
            bool                     MouseIsShown;      ///< Whether the mouse cursor is shown. Non-interactive GUIs normally don't show a cursor.


            // Methods called from Lua scripts on cf::GuiSys::GuiTs.
            static int Activate(lua_State* LuaState);           ///< Sets the IsActive flag of this GUI.
            static int Close(lua_State* LuaState);              ///< Same as calling "gui:activate(false);".
            static int SetInteractive(lua_State* LuaState);     ///< Sets the IsInteractive flag of this GUI.
            static int SetFullCover(lua_State* LuaState);       ///< Sets the IsFullCover flag of this GUI.
            static int SetMousePos(lua_State* LuaState);        ///< Sets the position of the mouse cursor.
            static int SetMouseCursorSize(lua_State* LuaState); ///< Sets the size of the mouse cursor.
            static int SetMouseMat(lua_State* LuaState);        ///< Sets the material that is used to render the mouse cursor.
            static int SetMouseIsShown(lua_State* LuaState);    ///< Determines whether the mouse cursor is shown at all.
            static int CreateNew(lua_State* LuaState);          ///< Creates and returns a new window or component.
            static int SetFocus(lua_State* LuaState);           ///< Sets the keyboard input focus to the given window. Does *not* call the Lua OnFocusLose() or OnFocusGain() scripts!
            static int GetRootWindow(lua_State* LuaState);      ///< Returns the root window of this GUI.
            static int SetRootWindow(lua_State* LuaState);      ///< Sets the root window for this GUI.
            static int Init(lua_State* LuaState);               ///< Calls the OnInit() script methods of all windows.
            static int toString(lua_State* LuaState);           ///< Returns a string representation of this GUI.

            static const luaL_Reg               MethodsList[];  ///< List of methods registered with Lua.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
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
