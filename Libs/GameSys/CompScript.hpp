/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMESYS_COMPONENT_SCRIPT_HPP_INCLUDED
#define CAFU_GAMESYS_COMPONENT_SCRIPT_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GameSys
    {
        /// This component runs custom Lua script code, implementing the behaviour of the entity in the game world.
        /// The script code can be loaded from a separate file, or it can be entered and kept directly in the component.
        ///
        /// Keeping the script code in a separate file is useful when it is re-used with several entity instances
        /// or in several maps.
        /// Keeping the script code directly in the component is useful for short scripts that are unique to a single
        /// map and entity instance.
        /// Note that both options can also be combined: The script code from a file is loaded first, and immediate
        /// code can be used to augment it (for example to "configure" it).
        class ComponentScriptT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentScriptT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentScriptT(const ComponentScriptT& Comp);

            /// This function is used for posting an event of the given type.
            /// It's the twin of `PostEvent(lua_State* LuaState)` below, but allows also C++ code to post events.
            /// It is assumed that in the script (e.g. "HumanPlayer.lua"), script method InitEventTypes() has been called.
            ///
            /// The event is automatically sent from the entity instance on the server to the entity instances
            /// on the clients, and causes a matching call to the ProcessEvent() callback there.
            /// The meaning of the event type is up to the script code that implements ProcessEvent().
            /// Note that events are fully predictable: they work well even in the presence of client prediction.
            void PostEvent(unsigned int EventType);


            // Base class overrides.
            ComponentScriptT* Clone() const;
            const char* GetName() const { return "Script"; }
            unsigned int GetEditorColor() const { return 0x4482FC; }
            void OnPostLoad(bool OnlyStatic);


            // The TypeSys related declarations for this class.
            const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
            static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
            static const cf::TypeSys::TypeInfoT TypeInfo;


            protected:

            // The Lua API methods of this class.
            static int InitEventTypes(lua_State* LuaState);
            static int PostEvent(lua_State* LuaState);
            static int DamageAll(lua_State* LuaState);
            static int toString(lua_State* LuaState);

            static const luaL_Reg               MethodsList[];  ///< The list of Lua methods for this class.
            static const char*                  DocClass;
            static const cf::TypeSys::MethsDocT DocMethods[];
            static const cf::TypeSys::MethsDocT DocCallbacks[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            void DoSerialize(cf::Network::OutStreamT& Stream) const /*override*/;
            void DoDeserialize(cf::Network::InStreamT& Stream, bool IsIniting) /*override*/;
            void DoServerFrame(float t) /*override*/;

            TypeSys::VarT<std::string> m_FileName;
            TypeSys::VarT<std::string> m_ScriptCode;

            ArrayT<unsigned int> m_EventsCount;   ///< A counter for each event type for the number of its occurrences. Serialized (and deserialized) normally along with the entity state.
            ArrayT<unsigned int> m_EventsRef;     ///< A reference counter for each event type for the number of processed occurrences. Never serialized (or deserialized), never reset, strictly growing.
        };
    }
}

#endif
