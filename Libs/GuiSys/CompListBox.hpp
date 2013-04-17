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

#ifndef CAFU_GUISYS_COMPONENT_LISTBOX_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_LISTBOX_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        class ComponentTextT;


        /// This components turns its window into a list-box control.
        /// It requires that in the same window a text component is available where the aspects of text rendering are
        /// configured (but that normally has empty text contents itself).
        class ComponentListBoxT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentListBoxT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentListBoxT(const ComponentListBoxT& Comp);

            // Base class overrides.
            ComponentListBoxT* Clone() const;
            const char* GetName() const { return "ListBox"; }
            void UpdateDependencies(WindowT* Window);
            void Render() const;
            bool OnInputEvent(const CaKeyboardEventT& KE);
            bool OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int GetSelItem(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            IntrusivePtrT<ComponentTextT>        m_TextComp;    ///< The sibling text component from which we take the text settings.
            TypeSys::VarT< ArrayT<std::string> > m_Items;       ///< The list of available items.
            TypeSys::VarT<unsigned int>          m_Selection;   ///< The index number of the currently selected item, where 1 corresponds to the first item (as per Lua convention). Use 0 for "no selection".
            TypeSys::VarT<Vector3fT>             m_BgColorOdd;  ///< The background color for odd rows.
            TypeSys::VarT<float>                 m_BgAlphaOdd;  ///< The background alpha for odd rows.
            TypeSys::VarT<Vector3fT>             m_BgColorEven; ///< The background color for even rows.
            TypeSys::VarT<float>                 m_BgAlphaEven; ///< The background alpha for even rows.
            TypeSys::VarT<Vector3fT>             m_BgColorSel;  ///< The background color for selected rows.
            TypeSys::VarT<float>                 m_BgAlphaSel;  ///< The background alpha for selected rows.
            TypeSys::VarT<Vector3fT>             m_TextColorSel;///< The foreground color for selected rows.
            TypeSys::VarT<float>                 m_TextAlphaSel;///< The foreground alpha for selected rows.
        };
    }
}

#endif
