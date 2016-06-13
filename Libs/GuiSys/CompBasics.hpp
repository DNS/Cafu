/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GUISYS_COMPONENT_WINDOW_BASICS_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_WINDOW_BASICS_HPP_INCLUDED

#include "CompBase.hpp"


namespace cf
{
    namespace GuiSys
    {
        /// This component adds the basics of the window (its name and the "is shown?" flag).
        /// It is one of the components that is "fundamental" to a window (cf::GuiSys::IsFundamental() returns `true`).
        /// Every window must have exactly one.
        class ComponentBasicsT : public ComponentBaseT
        {
            public:

            /// The constructor.
            ComponentBasicsT();

            /// The copy constructor.
            /// @param Comp   The component to create a copy of.
            ComponentBasicsT(const ComponentBasicsT& Comp);

            /// Returns the name of the window.
            const std::string& GetWindowName() const { return m_Name.Get(); }

            /// Returns `true` if the window is currently shown. Returns `false` if the window is currently hidden.
            bool IsShown() const { return m_Show.Get(); }

            /// Sets a new name for the window.
            /// Window names must be valid Lua script identifiers and unique among their siblings, and the method
            /// modifies the given string as necessary. As a result, GetWindowName() can return a string that is
            /// different from the string given to a preceeding call to SetWindowName().
            void SetWindowName(const std::string& Name) { m_Name.Set(Name); }

            // Base class overrides.
            ComponentBasicsT* Clone() const;
            const char* GetName() const { return "Basics"; }


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
            static const cf::TypeSys::MethsDocT DocCallbacks[];
            static const cf::TypeSys::VarsDocT  DocVars[];


            private:

            /// A variable of type `std::string`, specifically for window names.
            /// Window names must be valid Lua script identifiers and unique among their siblings.
            class WindowNameT : public TypeSys::VarT<std::string>
            {
                public:

                WindowNameT(const char* Name, const std::string& Value, const char* Flags[], ComponentBasicsT& CompBasics);
                WindowNameT(const WindowNameT& Var, ComponentBasicsT& CompBasics);

                // Base class overrides.
                void Set(const std::string& v);


                private:

                ComponentBasicsT& m_CompBasics; ///< The parent ComponentBasicsT that contains this variable.
            };

            /// A variable of type `bool`, indicating whether this window is currently shown or hidden.
            /// Besides keeping the boolean flag, this variable calls the `OnShow()` script callback whenever its value changes.
            class WindowShowT : public TypeSys::VarT<bool>
            {
                public:

                WindowShowT(const char* Name, const bool& Value, const char* Flags[], ComponentBasicsT& CompBasics);
                WindowShowT(const WindowShowT& Var, ComponentBasicsT& CompBasics);

                // Base class overrides.
                void Set(const bool& v);


                private:

                ComponentBasicsT& m_CompBasics; ///< The parent ComponentBasicsT that contains this variable.
            };


            WindowNameT m_Name;   ///< The name of the window. Window names must be valid Lua script identifiers and unique among their siblings.
            WindowShowT m_Show;   ///< Is this window currently shown?
        };
    }
}

#endif
