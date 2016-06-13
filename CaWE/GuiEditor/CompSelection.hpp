/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
