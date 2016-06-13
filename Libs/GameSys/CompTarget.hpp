/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_TARGET_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_TARGET_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component connects its entity to another.
        /// It is used by Script or GUI (Model) components in order to learn which other entity
        /// is related and should possibly be acted upon. For example, Target components are
        /// often used to let generic "open door" GUIs know which door they should actually open.
        class ComponentTargetT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentTargetT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentTargetT(const ComponentTargetT& Comp);

            // /// Returns ...
            // xy GetTargetEntity() const;


            // Base class overrides.
            ComponentTargetT* Clone() const;
            const char* GetName() const { return "Target"; }
            unsigned int GetEditorColor() const { return 0x0000FF; }
            BoundingBox3fT GetEditorBB() const { return BoundingBox3fT(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8)); }


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

            TypeSys::VarT<std::string> m_TargetName;
        };
    }
}

#endif
