/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_TRANSFORM_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_TRANSFORM_HPP_INCLUDED

#include "CompBase.hpp"
#include "Math3D/Matrix.hpp"
#include "Math3D/Quaternion.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component adds information about the position and orientation of its entity.
        /// Positions and orientations can be measured relative to several distinct spaces:
        ///
        /// world-space
        ///   : The global and "absolute" coordinate space that also exists when nothing else does.
        ///
        /// entity-space
        ///   : The local coordinate system of the entity. It is defined by the entity's transform component relative
        ///     to the entity's parent-space. The term "model-space" can be used synonymously with "entity-space".
        ///
        /// parent-space
        ///   : The entity-space of an entity's parent.
        ///     If an entity has no parent entity, this is the same as world-space.
        ///
        /// Although transform components can theoretically and technically exist without being attached to an entity,
        /// in practice this distinction is not made. Every entity has exactly one built-in transform component, and
        /// terms like "the origin of the transform" and "the origin of the entity" are used synonymously.
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

            /// Returns the origin of the transform (in "parent-space").
            const Vector3fT& GetOriginPS() const { return m_Origin.Get(); }

            /// Sets the origin of the transform (in "parent-space").
            void SetOriginPS(const Vector3fT& Origin) { m_Origin.Set(Origin); }

            /// Returns the orientation of the transform (in "parent-space").
            const cf::math::QuaternionfT GetQuatPS() const { return cf::math::QuaternionfT::FromXYZ(m_Quat.Get()); }

            /// Sets the orientation of the transform (in "parent-space").
            void SetQuatPS(const cf::math::QuaternionfT& Quat) { m_Quat.Set(Quat.GetXYZ()); }

            /// Returns the origin of the transform (in world-space).
            Vector3fT GetOriginWS() const;

            /// Sets the origin of the transform (in world-space).
            void SetOriginWS(const Vector3fT& OriginWS);

            /// Returns the orientation of the transform (in world-space).
            const cf::math::QuaternionfT GetQuatWS() const;

            /// Sets the orientation of the transform (in world-space).
            void SetQuatWS(const cf::math::QuaternionfT& QuatWS);

            /// Returns the transformation matrix from local entity-space to world-space.
            MatrixT GetEntityToWorld() const;

            /// Sets the orientation of the transform so that it "looks at" the given position.
            /// The new orientation is chosen such that the bank angle is always 0 relative to the xy-plane.
            /// @param Pos       The target position to look at.
            /// @param AxisNr    The "look axis", i.e. the number of the axis to orient towards `Pos`:
            ///                  0 for the x-axis, 1 for the y-axis.
            /// @param NoPitch   If `true`, the pitch angle is kept at 0, and the given axis points towards `Pos`
            ///                  only in the XY-Plane and the z-axis points straight up (0, 0, 1).
            void LookAt(const Vector3fT& Pos, unsigned int AxisNr = 0, bool NoPitch = false);


            // Base class overrides.
            ComponentTransformT* Clone() const;
            const char* GetName() const { return "Transform"; }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int GetOriginWS(lua_State* LuaState);
            static int SetOriginWS(lua_State* LuaState);
            static int GetAngles(lua_State* LuaState);
            static int SetAngles(lua_State* LuaState);
            static int GetAxisX(lua_State* LuaState);
            static int GetAxisY(lua_State* LuaState);
            static int GetAxisZ(lua_State* LuaState);
            static int LookAt(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            TypeSys::VarT<Vector3fT> m_Origin;  ///< The origin of the transform (in "parent-space").
            TypeSys::VarT<Vector3fT> m_Quat;    ///< The orientation of the transform (in "parent-space"), kept as the first three components (x, y, z) of a unit quaternion.
        };
    }
}

#endif
