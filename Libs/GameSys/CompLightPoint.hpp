/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_GAMESYS_COMPONENT_POINT_LIGHT_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_POINT_LIGHT_HPP_INCLUDED

#include "CompLight.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component adds a dynamic point light source to its entity.
        class ComponentPointLightT : public ComponentLightT
        {
            public:

            /// A variable of type int, specifically for the type of shadow that a light source casts.
            class VarShadowTypeT : public TypeSys::VarT<int>
            {
                public:

                enum { NONE = 0, STENCIL };

                VarShadowTypeT(const char* Name, const int& Value, const char* Flags[]=NULL);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const;
            };


            /// The constructor.
            ComponentPointLightT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentPointLightT(const ComponentPointLightT& Comp);

            bool IsOn() const { return m_On.Get(); }
            Vector3fT GetColor() const { return m_Color.Get(); }
            float GetRadius() const { return m_Radius.Get(); }
            bool CastsShadows() const { return m_ShadowType.Get() != VarShadowTypeT::NONE; }


            // Base class overrides.
            ComponentPointLightT* Clone() const override;
            const char* GetName() const override { return "PointLight"; }
            BoundingBox3fT GetCullingBB() const override;


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const override { return &TypeInfo; }
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

            TypeSys::VarT<bool>      m_On;
            TypeSys::VarT<Vector3fT> m_Color;
            TypeSys::VarT<float>     m_Radius;
            VarShadowTypeT           m_ShadowType;
        };
    }
}

#endif
