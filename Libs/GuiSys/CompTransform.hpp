/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUISYS_COMPONENT_TRANSFORM_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_TRANSFORM_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// This component adds information about the position and size of the window.
        /// It is one of the components that is "fundamental" to a window (cf::GuiSys::IsFundamental() returns `true`).
        /// Every window must have exactly one.
        class ComponentTransformT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentTransformT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentTransformT(const ComponentTransformT& Comp);

            const Vector2fT& GetPos() const { return m_Pos.Get(); }
            const Vector2fT& GetSize() const { return m_Size.Get(); }
            float GetRotAngle() const { return m_RotAngle.Get(); }

            void SetPos(const Vector2fT& Pos) { m_Pos.Set(Pos); }
            void SetSize(const Vector2fT& Size) { m_Size.Set(Size); }
            void SetRotAngle(float RotAngle) { m_RotAngle.Set(RotAngle); }

            // Base class overrides.
            ComponentTransformT* Clone() const;
            const char* GetName() const { return "Transform"; }


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

            enum SizeFlagsT { RATIO, FIXED };

            TypeSys::VarT<Vector2fT> m_Pos;       ///< The position of the top-left corner of the window, relative to its parent.
            TypeSys::VarT<Vector2fT> m_Size;      ///< The size of the window.
            TypeSys::VarT<float>     m_RotAngle;  ///< The angle in degrees by how much this entire window is rotated. Obsolete if we have 3D transforms?

            // SizeFlagsT HorzFlags[3];
            // SizeFlagsT VertFlags[3];
        };
    }
}

#endif
