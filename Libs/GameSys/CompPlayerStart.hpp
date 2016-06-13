/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_PLAYER_START_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_PLAYER_START_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component marks its entity as possible spawn point for human players
        /// that begin a single-player level or join a multi-player game.
        class ComponentPlayerStartT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentPlayerStartT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentPlayerStartT(const ComponentPlayerStartT& Comp);

            /// Returns whether players can be spawned here in single-player games.
            bool IsSinglePlayerStart() const { return m_SinglePlayer.Get(); }

            /// Returns whether players can be spawned here in multi-player games.
            bool IsMultiPlayerStart() const { return m_MultiPlayer.Get(); }


            // Base class overrides.
            ComponentPlayerStartT* Clone() const;
            const char* GetName() const { return "PlayerStart"; }
            unsigned int GetEditorColor() const { return 0x00FF00; }
            BoundingBox3fT GetEditorBB() const { return BoundingBox3fT(Vector3fT(-16, -16, -36), Vector3fT(16, 16, 36)); }


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

            TypeSys::VarT<bool> m_SinglePlayer;     ///< If true, players can be spawned here in single-player games.
            TypeSys::VarT<bool> m_MultiPlayer;      ///< If true, players can be spawned here in multi-player games.
        };
    }
}

#endif
