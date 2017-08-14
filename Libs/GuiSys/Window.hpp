/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUISYS_WINDOW_HPP_INCLUDED
#define CAFU_GUISYS_WINDOW_HPP_INCLUDED

#include "CompBasics.hpp"
#include "CompTransform.hpp"
#include "Templates/Array.hpp"
#include "Templates/Pointer.hpp"

#include <climits>


struct CaKeyboardEventT;
struct CaMouseEventT;
struct lua_State;
struct luaL_Reg;
namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class TypeInfoManT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }
namespace cf { namespace TypeSys { class MethsDocT; } }


namespace cf
{
    namespace GuiSys
    {
        class ComponentBaseT;
        class GuiImplT;
        class WindowCreateParamsT;


        /// The TypeInfoTs of all WindowT derived classes must register with this TypeInfoManT instance.
        cf::TypeSys::TypeInfoManT& GetWindowTIM();


        /**
         * This class represents a window of the GuiSys.
         * A WindowT is the most basic element of a GUI, and all other windows are derived and/or combined from this.
         *
         * WindowT instances can be created in C++ code or in Lua scripts, using the gui:new() function.
         * They can be passed from C++ code to Lua and from Lua to C++ code at will.
         * In C++ code, all WindowT instances are kept in IntrusivePtrT's. Their lifetime is properly managed:
         * A window is deleted automatically when it is no longer used in Lua \emph{and} in C++.
         * That is, code like
         *     Example 1:    w=gui:new("WindowT"); gui:SetRootWindow(w); w=nil;
         *     Example 2:    w=gui:new("WindowT"); w:AddChild(gui:new("WindowT"));
         * works as expected. See the cf::ScriptBinderT class for technical and implementation details.
         */
        class WindowT : public RefCountedT
        {
            public:

            /// The normal constructor.
            /// @param Params   The creation parameters for the window.
            WindowT(const WindowCreateParamsT& Params);

            /// The copy constructor.
            /// Copies a window (optionally with all of its children recursively).
            /// The parent of the copy is always NULL and it is up to the caller to put the copy into a window hierarchy.
            /// @param Window      The window to construct this window from.
            /// @param Recursive   Whether to recursively copy all children.
            WindowT(const WindowT& Window, bool Recursive=false);

            /// The destructor. Destructs this window and all its children.
            ~WindowT();


            GuiImplT& GetGui() const { return m_Gui; }

            /// Returns the parent window of this window.
            IntrusivePtrT<WindowT> GetParent() const { return m_Parent; }

            /// Returns the immediate children of this window.
            /// This is analogous to calling GetChildren(Chld, false) with an initially empty Chld array.
            const ArrayT< IntrusivePtrT<WindowT> >& GetChildren() const { return m_Children; }

            /// Returns the children of this window.
            /// @param Chld      The array to which the children of this window are appended. Note that Chld gets *not* initially cleared by this function!
            /// @param Recurse   Determines if also the grand-children, grand-grand-children etc. are returned.
            void GetChildren(ArrayT< IntrusivePtrT<WindowT> >& Chld, bool Recurse=false) const;

            /// Adds the given window to the children of this window, and sets this window as the parent of the child.
            ///
            /// This method also makes sure that the name of the Child is unique among its siblings,
            /// modifying it as necessary. See SetName() for more details.
            ///
            /// @param Child   The window to add to the children of this window.
            /// @param Pos     The position among the children to insert the child window at.
            /// @returns true on success, false on failure (Child has a parent already, or is the root of this window).
            bool AddChild(IntrusivePtrT<WindowT> Child, unsigned long Pos=0xFFFFFFFF);

            /// Removes the given window from the children of this window.
            /// @param Child   The window to remove from the children of this window.
            /// @returns true on success, false on failure (Child is not a child of this window).
            bool RemoveChild(IntrusivePtrT<WindowT> Child);

            /// Returns the top-most parent of this window, that is, the root of the hierarchy this window is in.
            IntrusivePtrT<WindowT> GetRoot();     // Method cannot be const because return type is not const -- see implementation.


            /// Returns the application component of this window.
            /// This component is much like the "Basics" and "Transform" components, but it can be set by the
            /// application (see SetApp()), and is intended for the sole use by the application, e.g. for
            /// implementing a "selection gizmo" in the GUI Editor.
            IntrusivePtrT<ComponentBaseT> GetApp() { return m_App; }

            /// The `const` variant of the GetApp() method above. See GetApp() for details.
            IntrusivePtrT<const ComponentBaseT> GetApp() const { return m_App; }

            /// Sets the application component for this window. See GetApp() for details.
            void SetApp(IntrusivePtrT<ComponentBaseT> App);

            /// Returns the "Basics" component of this window.
            /// The "Basics" component defines the name and the "show" flag of the window.
            IntrusivePtrT<ComponentBasicsT> GetBasics() const { return m_Basics; }

            /// Returns the "Transform" component of this window.
            /// The "Transform" component defines the position, size and orientation of the window.
            IntrusivePtrT<ComponentTransformT> GetTransform() const { return m_Transform; }


            /// Returns the components that this window is composed of.
            /// Only the "custom" components are returned, does *not* include the application component,
            /// "Basics" or "Transform".
            const ArrayT< IntrusivePtrT<ComponentBaseT> >& GetComponents() const { return m_Components; }

            /// Returns the (n-th) component of the given (type) name.
            /// Covers the "custom" components as well as the application components, "Basics" and "Transform".
            /// That is, `GetComponent("Basics") == GetBasics()` and `GetComponent("Transform") == GetTransform()`.
            IntrusivePtrT<ComponentBaseT> GetComponent(const std::string& TypeName, unsigned int n=0) const;

            /// Adds the given component to this window.
            ///
            /// @param Comp    The component to add to this window.
            /// @param Index   The position among the other components to insert `Comp` at.
            /// @returns `true` on success, `false` on failure (if `Comp` is part of a window already).
            bool AddComponent(IntrusivePtrT<ComponentBaseT> Comp, unsigned long Index=ULONG_MAX);

            /// Deletes the component at the given index from this window.
            void DeleteComponent(unsigned long CompNr);


            /// Returns the position of the upper left corner of this window in absolute (vs. relative to the parent) coordinates.
            Vector2fT GetAbsolutePos() const;

            /// Finds the window with the name WantedName in the hierachy tree of this window.
            /// Use GetRoot()->Find("xy") in order to search the entire GUI for the window with name "xy".
            /// @param WantedName   The name of the window that is to be found.
            /// @returns The pointer to the desired window, or NULL if no window with this name exists.
            IntrusivePtrT<WindowT> Find(const std::string& WantedName);   // Method cannot be const because return type is not const -- see implementation.

            /// Finds the topmost window that contains the point `Pos` in the hierachy tree of this window
            /// (with `Pos` being in (absolute) screen coordinates, *not* relative to this window).
            /// Use `GetRoot()->Find(Pos)` in order to search the entire GUI for the window containing the point `Pos`.
            /// @param Pos   The coordinate of the point to test.
            /// @param OnlyVisible   If true, only visible windows are reported. If false, all windows are searched.
            /// @returns The pointer to the desired window, or NULL if there is no window that contains `Pos`.
            IntrusivePtrT<WindowT> Find(const Vector2fT& Pos, bool OnlyVisible=true); // Method cannot be const because return type is not const -- see implementation.

            /// Writes the current state of this window into the given stream.
            /// This method is called to send the state of the window over the network, to save it to disk,
            /// or to store it in the clipboard.
            ///
            /// @param Stream
            ///   The stream to write the state data to.
            ///
            /// @param WithChildren
            ///   Should the children be recursively serialized as well?
            void Serialize(cf::Network::OutStreamT& Stream, bool WithChildren=false) const;

            /// Reads the state of this window from the given stream, and updates the window accordingly.
            /// This method is called after the state of the window has been received over the network,
            /// has been loaded from disk, has been read from the clipboard, or must be "reset" for the purpose
            /// of (re-)prediction.
            ///
            /// @param Stream
            ///   The stream to read the state data from.
            ///
            /// @param IsIniting
            ///   Used to indicate that the call is part of the construction / first-time initialization of the window.
            ///   The implementation will use this to not wrongly process the event counters, interpolation, etc.
            void Deserialize(cf::Network::InStreamT& Stream, bool IsIniting);

            /// Renders this window.
            /// Note that this method does *not* setup any of the MatSys's model, view or projection matrices: it's up to the caller to do that!
            void Render() const;

            /// Keyboard input event handler.
            /// @param KE Keyboard event.
            bool OnInputEvent(const CaKeyboardEventT& KE);

            /// Mouse input event handler.
            /// @param ME Mouse event.
            /// @param PosX Mouse position x.
            /// @param PosY Mouse position y.
            bool OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY);

            /// The clock-tick event handler.
            /// @param t The time in seconds since the last clock-tick.
            bool OnClockTickEvent(float t);

            /// Calls the Lua method with name `MethodName` of this window.
            /// This method is analogous to GuiI::CallLuaFunc(), see there for more details.
            /// @param MethodName The name of the lua method to call.
            /// @param Signature DOCTODO
            bool CallLuaMethod(const char* MethodName, const char* Signature="", ...);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // Methods called from Lua scripts on cf::GuiSys::WindowTs.
            // They are protected so that derived window classes can access them when implementing overloads.
            static int AddChild(lua_State* LuaState);
            static int RemoveChild(lua_State* LuaState);
            static int GetParent(lua_State* LuaState);
            static int GetChildren(lua_State* LuaState);
            static int GetTime(lua_State* LuaState);
            static int GetBasics(lua_State* LuaState);
            static int GetTransform(lua_State* LuaState);
            static int AddComponent(lua_State* LuaState);
            static int RmvComponent(lua_State* LuaState);
            static int GetComponents(lua_State* LuaState);
            static int GetComponent(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< List of methods registered with Lua.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::MethsDocT DocCallbacks[];


            private:

            void operator = (const WindowT&);   ///< Use of the Assignment Operator is not allowed.

            GuiImplT&                               m_Gui;          ///< The GUI instance in which this window was created and exists. Useful in many regards, but especially for access to the underlying Lua state.
            WindowT*                                m_Parent;       ///< The parent of this window. May be NULL if there is no parent. In order to not create cycles of IntrusivePtrT's, the type is intentionally a raw pointer only.
            ArrayT< IntrusivePtrT<WindowT> >        m_Children;     ///< The list of children of this window.
            float                                   m_Time;         ///< The local time (starting at 0.0) of this window.
            IntrusivePtrT<ComponentBaseT>           m_App;          ///< A component for the sole use by the application / implementation.
            IntrusivePtrT<ComponentBasicsT>         m_Basics;       ///< The component that defines the name and the "show" flag of this window.
            IntrusivePtrT<ComponentTransformT>      m_Transform;    ///< The component that defines the position, size and orientation of this window.
            ArrayT< IntrusivePtrT<ComponentBaseT> > m_Components;   ///< The components that this window is composed of.
        };
    }
}

#endif
