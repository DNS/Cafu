/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUISYS_COMPONENT_BORDER_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_BORDER_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// This components adds a border to its window.
        class ComponentBorderT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentBorderT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentBorderT(const ComponentBorderT& Comp);

            // Base class overrides.
            ComponentBorderT* Clone() const;
            const char* GetName() const { return "Border"; }
            void Render() const;


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void FillMemberVars();  ///< A helper method for the constructors.

            TypeSys::VarT<float>     m_Width;   ///< The width of the border.
            TypeSys::VarT<Vector3fT> m_Color;   ///< The border color.
            TypeSys::VarT<float>     m_Alpha;   ///< The alpha component of the color.
        };
    }
}

#endif
