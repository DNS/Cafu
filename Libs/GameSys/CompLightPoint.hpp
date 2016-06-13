/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
