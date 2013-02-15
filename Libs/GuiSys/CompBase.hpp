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

#ifndef CAFU_GUISYS_COMPONENT_BASE_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_BASE_HPP_INCLUDED

#include "Variables.hpp"
#include "Templates/Pointer.hpp"


namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }

struct CaKeyboardEventT;
struct CaMouseEventT;
struct lua_State;
struct luaL_Reg;


namespace cf
{
    namespace GuiSys
    {
        class WindowT;

        /// This is the base class for the components that a window is composed/aggregated of.
        ///
        /// Components are the basic building blocks of a window: their composition defines
        /// the properties, the behaviour, and thus virtually every aspect of the window.
        ///
        /// Components can exist in two invariants:
        ///   - Stand-alone, independent and not a part of any window.
        ///   - Normally, as an active part of a window.
        ///
        /// Stand-alone components typically occur when they're newly instantiated, for example
        /// when they are loaded from disk, when they are instantiated in scripts, or when they
        /// are kept in the clipboard or managed in the Undo/Redo system of the GUI Editor.
        /// Newly created, copied or cloned components are initially stand-alone.
        ///
        /// A component becomes a part of a window via the WindowT::AddComponent() method.
        /// The window then knows the component, because it hosts it, and reversely, the
        /// component then knows the parent window that it is a component of.
        class ComponentBaseT : public RefCountedT
        {
            public:

            /// The constructor.
            /// The newly created component is initially not a part of any window.
            ComponentBaseT();

            /// The copy constructor.
            /// The newly copied component is initially not a part of any window, even if the source component was.
            /// @param Comp   The component to create a copy of.
            ComponentBaseT(const ComponentBaseT& Comp);

            /// The virtual copy constructor.
            /// Callers can use this method to create a copy of this component without knowing its concrete type.
            /// Overrides in derived classes use a covariant return type to facilitate use when the concrete type is known.
            /// The newly cloned component is initially not a part of any window, even if the source component was.
            virtual ComponentBaseT* Clone() const;

            /// The virtual destructor.
            virtual ~ComponentBaseT() { }


            /// Returns the parent window that contains this component,
            /// or `NULL` if this component is currently not a part of any window.
            WindowT* GetWindow() const { return m_Window; }

            /// Returns the variable manager that keeps generic references to our member variables,
            /// providing a simple kind of "reflection" or "type introspection" feature.
            TypeSys::VarManT& GetMemberVars() { return m_MemberVars; }

            /// Calls the given Lua method of this component.
            /// This method is analogous to UniScriptStateT::CallMethod(), see there for details.
            /// @param MethodName   The name of the Lua method to call.
            /// @param Signature    See UniScriptStateT::Call() for details.
            bool CallLuaMethod(const char* MethodName, const char* Signature="", ...);

            /// Returns the name of this component.
            virtual const char* GetName() const { return "Base"; }

            /// This method is called whenever something "external" to this component has changed:
            ///   - if the parent window has changed, because this window was added to or removed from it,
            ///   - if other components in the parent window have changed.
            /// The component can use the opportunity to search the window for "sibling" components
            /// that it depends on, and store direct pointers to them.
            /// Note however that dependencies among components must not be cyclic, or else the deletion
            /// of a window will leave a memory leak.
            /// @param Window   The parent window that contains this component, or `NULL` to indicate that this component is removed from the window that it used to be a part of.
            virtual void UpdateDependencies(WindowT* Window);

            /// This method implements the graphical output of this component.
            virtual void Render() const { }

            /// This method is called after all windows and their components have been loaded.
            ///
            /// It is called only once when the static part of GUI initializatzion is complete, i.e. after the initial
            /// values of all windows and their components have been set.
            /// Components can override this method in order act / do something / add custom behaviour at that time.
            ///
            /// For example, a choice component can use it to set the associated text component to the initial
            /// selection, a script component can forward it to the script by calling a related script function, etc.
            ///
            /// @param InEditor   `true` if this GUI is instantiated in the GUI Editor. This normally means that
            ///     custom behaviour should *not* run. If `false`, this GUI is instantiated "live", in-game.
            virtual void OnPostLoad(bool InEditor) { }

            /// This method handles keyboard input events.
            /// @param KE   Keyboard event instance.
            /// @returns Whether the component handled ("consumed") the event.
            virtual bool OnInputEvent(const CaKeyboardEventT& KE) { return false; }

            /// This method handles mouse input events.
            /// @param ME     Mouse event instance.
            /// @param PosX   x-coordinate of the mouse cursor position.
            /// @param PosY   y-coordinate of the mouse cursor position.
            /// @returns Whether the component handled ("consumed") the event.
            virtual bool OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY) { return false; }

            /// This method handles clock-tick events.
            /// @param t   The time in seconds since the last clock-tick.
            virtual void OnClockTickEvent(float t) { }


            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            void operator = (const ComponentBaseT&);    ///< Use of the Assignment Operator is not allowed.

            // The Lua API methods of this class.
            static const luaL_Reg MethodsList[];              ///< The list of Lua methods for this class.
            static int Get(lua_State* LuaState);              ///< Gets a member variable of this class.
            static int Set(lua_State* LuaState);              ///< Sets a member variable of this class.
            static int GetExtraMessage(lua_State* LuaState);  ///< Returns the result of VarBaseT::GetExtraMessage() for the given member variable.
            static int toString(lua_State* LuaState);         ///< Returns a string representation of this object.

            WindowT*         m_Window;      ///< The parent window that contains this component, or `NULL` if this component is currently not a part of any window.
            TypeSys::VarManT m_MemberVars;  ///< The variable manager that keeps generic references to our member variables.
        };
    }
}

#endif
