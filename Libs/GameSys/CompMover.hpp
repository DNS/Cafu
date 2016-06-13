/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_MOVER_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_MOVER_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component controls the movement of one or more entities and implements the related effects.
        ///
        /// The component can handle a single entity, e.g. a moving platform or a lift, or several entities
        /// that act together as a team, e.g. the wings of a door.
        ///
        /// This component works in concert with a Script component, which must be present in the same entity.
        /// The Mover component queries the Script component for the desired spatial transformation of each
        /// team member, and notifies it whenever the movement is blocked by another entity (e.g. a player).
        /// It also implements the appropriate effects on other entities, e.g. their being pushed by moving
        /// parts, or their riding on top of them.
        class ComponentMoverT : public ComponentBaseT
        {
            public:

            /// A variable of type `int` that describes the mover's behavior when it is activated at the "dest" position.
            class VarDestActivatedT : public TypeSys::VarT<int>
            {
                public:

                enum { DESTACT_MOVE_HOME = 0, DESTACT_RESET_TIMEOUT, DESTACT_IGNORE };

                VarDestActivatedT(const char* Name, const int& Value, const char* Flags[]=NULL);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const override;
            };

            /// A variable of type `int` that describes the mover's behavior regarding other entities.
            class VarOtherEntitiesT : public TypeSys::VarT<int>
            {
                public:

                enum { OTHERENTS_IGNORE = 0, OTHERENTS_CANNOT_PUSH, OTHERENTS_CAN_PUSH, OTHERENTS_CAN_FORCE_PUSH };

                VarOtherEntitiesT(const char* Name, const int& Value, const char* Flags[]=NULL);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const override;
            };

            /// A variable of type `int` that describes the base function that is used to compute the mover's trajectory.
            class VarTrajFuncT : public TypeSys::VarT<int>
            {
                public:

                enum { TRAJFUNC_LINEAR = 0, TRAJFUNC_SINE };

                VarTrajFuncT(const char* Name, const int& Value, const char* Flags[]=NULL);

                // Base class overrides.
                void GetChoices(ArrayT<std::string>& Strings, ArrayT<int>& Values) const override;
            };


            /// The constructor.
            ComponentMoverT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentMoverT(const ComponentMoverT& Comp);


            // Base class overrides.
            ComponentMoverT* Clone() const override;
            const char* GetName() const override { return "Mover"; }
            unsigned int GetEditorColor() const override { return 0xA00000; }
            BoundingBox3fT GetEditorBB() const override { return BoundingBox3fT(Vector3fT(-8, -8, -8), Vector3fT(8, 8, 8)); }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const override { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int HandleMove(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            bool HandleMove(float t) const;

            TypeSys::VarT<float> m_MoveDuration;    ///< The time in seconds that it takes to move each part from one endpoint to the other.
            VarDestActivatedT    m_DestActivated;   ///< Describes the mover's behavior when it is activated at the "dest" position.
            TypeSys::VarT<float> m_DestTimeout;     ///< The timeout in seconds after which the parts move back to their "home" position. A negative value to disables the timeout.
            VarOtherEntitiesT    m_OtherEntities;   ///< Describes the mover's behavior regarding other entities.
            VarTrajFuncT         m_TrajFunc;        ///< Describes the base function that is used to compute the mover's trajectory.
            TypeSys::VarT<float> m_TrajExp;         ///< The exponent that is applied to `trajFunc`.
        };
    }
}

#endif
