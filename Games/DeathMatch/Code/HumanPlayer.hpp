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

#ifndef CAFU_HUMAN_PLAYER_HPP_INCLUDED
#define CAFU_HUMAN_PLAYER_HPP_INCLUDED

#include "BaseEntity.hpp"


namespace cf { namespace GameSys { class ComponentModelT; } }
namespace cf { namespace GuiSys { class GuiImplT; } }
struct luaL_Reg;


namespace GAME_NAME
{
    class EntHumanPlayerT : public BaseEntityT
    {
        public:

        // Publicly defined enum for access from the "carried weapons".
        enum EventTypesT { EVENT_TYPE_PRIMARY_FIRE, EVENT_TYPE_SECONDARY_FIRE, NUM_EVENT_TYPES };

        EntHumanPlayerT(const EntityCreateParamsT& Params);
        ~EntHumanPlayerT();

        /// Increases the frag count of this entity by the given number.
        void AddFrag(int NumFrags=1);

        // Implement the BaseEntityT interface.
        void TakeDamage(BaseEntityT* Entity, char Amount, const VectorT& ImpactDir);
        void Think(float FrameTime, unsigned long ServerFrameNr);

        void ProcessEvent(unsigned int EventType, unsigned int NumEvents);
        void Draw(bool FirstPersonView, float LodDist) const;
        void PostDraw(float FrameTime, bool FirstPersonView);


        const cf::TypeSys::TypeInfoT* GetType() const;
        static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
        static const cf::TypeSys::TypeInfoT TypeInfo;


        private:

        // Override the base class methods.
        void NotifyLeaveMap();

        /// A helper function for Think().
        bool CheckGUI(IntrusivePtrT<cf::GameSys::ComponentModelT> CompModel, Vector3fT& MousePos) const;

        // Script methods.
        static int GetHealth(lua_State* LuaState);
        static int GetArmor(lua_State* LuaState);
        static int GetFrags(lua_State* LuaState);
        static int GetCrosshair(lua_State* LuaState);
        static int GetAmmoString(lua_State* LuaState);

        static const luaL_Reg MethodsList[];


        mutable VectorT   LastSeenAmbientColor;     // This is a client-side variable, unrelated to prediction, and thus allowed.
        IntrusivePtrT<cf::GuiSys::GuiImplT> GuiHUD; ///< The HUD GUI for this local human player entity.
    };
}

#endif
