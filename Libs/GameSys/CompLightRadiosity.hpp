/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_RADIOSITY_LIGHT_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_RADIOSITY_LIGHT_HPP_INCLUDED

#include "CompLight.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component adds a radiosity point light source to its entity.
        class ComponentRadiosityLightT : public ComponentLightT
        {
            public:

            /// The constructor.
            ComponentRadiosityLightT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentRadiosityLightT(const ComponentRadiosityLightT& Comp);

            Vector3fT GetColor() const { return m_Color.Get(); }
            float GetIntensity() const { return m_Intensity.Get(); }
            float GetConeAngle() const { return m_ConeAngle.Get(); }


            // Base class overrides.
            ComponentRadiosityLightT* Clone() const;
            const char* GetName() const { return "RadiosityLight"; }


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

            TypeSys::VarT<Vector3fT> m_Color;
            TypeSys::VarT<float>     m_Intensity;
            TypeSys::VarT<float>     m_ConeAngle;
        };
    }
}

#endif
