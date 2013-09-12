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

#ifndef CAFU_GAMESYS_COMPONENT_TRANSFORM_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_TRANSFORM_HPP_INCLUDED

#include "CompBase.hpp"
#include "Math3D/Quaternion.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component adds information about the position and orientation of the entity.
        /// It is one of the components that is "fundamental" to an entity (cf::GameSys::IsFundamental() returns `true`).
        /// Every entity must have exactly one.
        class ComponentTransformT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentTransformT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentTransformT(const ComponentTransformT& Comp);

            /// Returns whether the transformation described by this component is the identity ("no") transform.
            bool IsIdentity() const { return m_Origin.Get() == Vector3fT() && m_Quat.Get() == Vector3fT(); }

            /// Returns the origin of the entity (in the coordinate system of its parent).
            const Vector3fT& GetOrigin() const { return m_Origin.Get(); }

            /// Sets the origin of the entity (in the coordinate system of its parent).
            void SetOrigin(const Vector3fT& Origin) { m_Origin.Set(Origin); }

            /// Returns the orientation of the entity (in the coordinate system of its parent).
            /// The class keeps the orientation as the first three components (x, y, z) of a unit quaternion,
            /// so the method has to convert appropriately.
            const cf::math::QuaternionfT GetQuat() const { return cf::math::QuaternionfT::FromXYZ(m_Quat.Get()); }

            /// Sets the orientation of the entity (in the coordinate system of its parent).
            /// The class keeps the orientation as the first three components (x, y, z) of a unit quaternion,
            /// so the method has to convert appropriately.
            void SetQuat(const cf::math::QuaternionfT& Quat) { m_Quat.Set(Quat.GetXYZ()); }


            // Base class overrides.
            ComponentTransformT* Clone() const;
            const char* GetName() const { return "Transform"; }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int GetAngles(lua_State* LuaState);
            static int SetAngles(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            TypeSys::VarT<Vector3fT> m_Origin;  ///< The origin of the entity (in the coordinate system of its parent).
            TypeSys::VarT<Vector3fT> m_Quat;    ///< The orientation of the entity (in the coordinate system of its parent).
        };
    }
}

#endif
