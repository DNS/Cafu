/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
