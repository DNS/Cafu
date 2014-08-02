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

#ifndef CAFU_GAMESYS_COMPONENT_PARTICLE_SYSTEM_OLD_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_PARTICLE_SYSTEM_OLD_HPP_INCLUDED

#include "CompBase.hpp"


namespace MatSys { class RenderMaterialT; }


namespace cf
{
    namespace GameSys
    {
        /// This component adds a particle system to its entity.
        /// The particle system is obsolete though: This is just a quick and dirty port
        /// of the particle system in the old game system to the new component system.
        /// Both its interface and its implementation need a thorough overhaul.
        class ComponentParticleSystemOldT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentParticleSystemOldT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentParticleSystemOldT(const ComponentParticleSystemOldT& Comp);

            /// The destructor.
            ~ComponentParticleSystemOldT();


            // Base class overrides.
            ComponentParticleSystemOldT* Clone() const;
            const char* GetName() const { return "ParticleSystemOld"; }
            unsigned int GetEditorColor() const { return 0xFFFF00; }


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int EmitParticle(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void InitRenderMats();

            TypeSys::VarT<std::string> m_Type;  ///< The type of the particles emitted by this system.

            ArrayT<MatSys::RenderMaterialT*> m_RenderMats;
        };
    }
}

#endif
