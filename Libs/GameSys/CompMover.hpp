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

            TypeSys::VarT<bool> m_CanPush;
        };
    }
}

#endif
