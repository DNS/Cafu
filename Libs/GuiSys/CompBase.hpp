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

#include "Templates/Pointer.hpp"


namespace cf { namespace TypeSys { class TypeInfoT; } }
namespace cf { namespace TypeSys { class CreateParamsT; } }

struct lua_State;
struct luaL_Reg;


namespace cf
{
    namespace GuiSys
    {
        class WindowT;

        /// This is the base class for the components that a window is composed/aggregated of.
        class ComponentBaseT : public RefCountedT
        {
            public:

            /// The constructor.
            /// @param Window   The window that the new component becomes a part of.
            ComponentBaseT(WindowT& Window);

            /// The copy constructor.
            /// The new component can become a part of the same or a different window than the component it was copied from.
            /// @param Comp     The component to create a copy of.
            /// @param Window   The window that the new component becomes a part of.
            ComponentBaseT(const ComponentBaseT& Comp, WindowT& Window);

            /// The virtual copy constructor.
            /// Callers can use this method to create a copy of this component without knowing its concrete type.
            /// Overrides in derived classes use a covariant return type to facilitate use when the concrete type is known.
            /// The new component can become a part of the same or a different window than the component it was copied from.
            /// @param Window   The window that the new component becomes a part of.
            virtual ComponentBaseT* Clone(WindowT& Window) const;

            /// The virtual destructor.
            virtual ~ComponentBaseT() { }


            /// Returns the parent window that contains this component.
            WindowT& GetWindow() const { return m_Window; }

            /// Returns the name of this component.
            virtual const char* GetName() const { return "Base"; }

            /// This method is called whenever the components in the parent window have changed,
            /// typically after the window has been loaded for the first time, has been created
            /// as a copy of another window, or has been edited in the GUI editor.
            /// The component can use the opportunity to search the window for "sibling" components
            /// that it depends on, and store direct pointers to them.
            /// Note however that dependencies among components must not be cyclic, or else the deletion
            /// of a window will leave a memory leak.
            virtual void UpdateDependencies() { }


            // The TypeSys related declarations for this class.
            virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            private:

            void operator = (const ComponentBaseT&);    ///< Use of the Assignment Operator is not allowed.

            // The Lua API methods of this class.
            static const luaL_Reg MethodsList[];        ///< The list of Lua methods for this class.
            static int toString(lua_State* LuaState);   ///< Returns a string representation of this object.

            WindowT& m_Window;    ///< The parent window that contains this component.
        };
    }
}

#endif
