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

#ifndef CAFU_GAMESYS_COMPONENT_SOUND_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_SOUND_HPP_INCLUDED

#include "CompBase.hpp"


class SoundI;


namespace cf
{
    namespace GameSys
    {
        /// This component adds 3D sound output to its entity.
        class ComponentSoundT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentSoundT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentSoundT(const ComponentSoundT& Comp);

            /// The destructor.
            ~ComponentSoundT();

            // Base class overrides.
            ComponentSoundT* Clone() const;
            const char* GetName() const { return "Sound"; }
            void DoClientFrame(float t);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int Play(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            SoundI* GetSound();

            TypeSys::VarT<std::string> m_Name;      ///< The name of the sound shader or sound file to play.
            TypeSys::VarT<bool>        m_AutoPlay;  ///< Whether the sound is played automatically in interval-spaced loops. If `false`, playbacks of the sound must be triggered by explicit calls to the Play() method.
            TypeSys::VarT<float>       m_Interval;  ///< If `m_AutoPlay` is `true`, this is the time in seconds between successive playbacks of the sound.

            std::string                m_PrevName;  ///< The previous file name, used to detect changes in `m_Name`.
            SoundI*                    m_Sound;     ///< The sound instance of this component, NULL for none.
            float                      m_PauseLeft; ///< If `m_AutoPlay` is `true`, how much of `m_Interval` is left before the sound is played again?
        };
    }
}

#endif
