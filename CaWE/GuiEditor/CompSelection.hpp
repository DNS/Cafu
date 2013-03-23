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

#ifndef CAFU_GUIEDITOR_COMPONENT_SELECTION_HPP_INCLUDED
#define CAFU_GUIEDITOR_COMPONENT_SELECTION_HPP_INCLUDED

#include "GuiSys/CompBase.hpp"


namespace GuiEditor
{
    /// This components reflects the selection state of its window.
    class ComponentSelectionT : public cf::GuiSys::ComponentBaseT
    {
        public:

        /// The constructor.
        ComponentSelectionT();

        /// The copy constructor.
        /// @param Comp   The component to create a copy of.
        ComponentSelectionT(const ComponentSelectionT& Comp);

        bool IsSelected() const { return m_IsSelected; }
        void SetSelected(bool Sel) { m_IsSelected = Sel; }

        // Base class overrides.
        ComponentSelectionT* Clone() const;
        const char* GetName() const { return "Selection"; }
        void Render() const;


        // The TypeSys related declarations for this class.
        const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
        static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
        static const cf::TypeSys::TypeInfoT TypeInfo;


        private:

        // The Lua API methods of this class.
        static const luaL_Reg MethodsList[];        ///< The list of Lua methods for this class.
        static int toString(lua_State* LuaState);   ///< Returns a string representation of this object.

        bool m_IsSelected;    ///< Is this window currently selected?
    };
}

#endif
