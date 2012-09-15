/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

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

#ifndef CAFU_SPEAKER_HPP_INCLUDED
#define CAFU_SPEAKER_HPP_INCLUDED

#include "BaseEntity.hpp"


class SoundI;
struct luaL_Reg;


namespace GAME_NAME
{
    class EntSpeakerT : public BaseEntityT
    {
        public:

        // Constructor
        EntSpeakerT(const EntityCreateParamsT& Params);

        // Destructor
        ~EntSpeakerT();

        // TypeInfo stuff.
        const cf::TypeSys::TypeInfoT* GetType() const;
        static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
        static const cf::TypeSys::TypeInfoT TypeInfo;

        // BaseEntityT implementation.
        virtual void PostDraw(float FrameTime, bool FirstPersonView);
        virtual void ProcessEvent(unsigned int EventType, unsigned int NumEvents);

        // Scripting methods.
        static int Play(lua_State* LuaState);
        static int Stop(lua_State* LuaState);


        private:

        enum EventTypesT { EVENT_TYPE_PLAY, EVENT_TYPE_STOP, NUM_EVENT_TYPES };

        // Override the base class methods.
        void DoSerialize(cf::Network::OutStreamT& Stream) const;
        void DoDeserialize(cf::Network::InStreamT& Stream);

        bool    m_AutoPlay;
        float   m_Interval;                  ///< Interval between two sound playbacks. 0 means the sound is only played one time if triggered.
        float   m_TimeUntilNextSound;        ///< Time left until the sound is played another time.
        SoundI* m_Sound;                     ///< The sound object to play.

        static const luaL_Reg MethodsList[]; ///< List of methods that can be called from Lua scripts.
    };
}

#endif
