/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
            unsigned int GetEditorColor() const { return 0xFF0000; }
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
