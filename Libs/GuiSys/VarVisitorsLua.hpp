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

#ifndef CAFU_GUISYS_VAR_VISITORS_LUA_HPP_INCLUDED
#define CAFU_GUISYS_VAR_VISITORS_LUA_HPP_INCLUDED

#include "Variables.hpp"


struct lua_State;


namespace cf
{
    namespace GuiSys
    {
        /// This visitor is used to implement a "get()" function in Lua:
        /// It pushes the value(s) of the visited variable onto the Lua stack.
        class VarVisitorGetToLuaT : public cf::TypeSys::VisitorConstT
        {
            public:

            VarVisitorGetToLuaT(lua_State* LuaState);

            unsigned int GetNumResults() const { return m_NumResults; }

            void visit(const cf::TypeSys::VarT<float>& Var);
            void visit(const cf::TypeSys::VarT<double>& Var);
            void visit(const cf::TypeSys::VarT<int>& Var);
            void visit(const cf::TypeSys::VarT<unsigned int>& Var);
            void visit(const cf::TypeSys::VarT<std::string>& Var);
            void visit(const cf::TypeSys::VarT<Vector3fT>& Var);
            void visit(const cf::TypeSys::VarT< ArrayT<std::string> >& Var);


            private:

            lua_State*   m_LuaState;
            unsigned int m_NumResults;
        };


        /// This visitor is used to implement a "set()" function in Lua:
        /// It sets the value of the visited variable to the value(s) taken from the Lua stack.
        class VarVisitorSetFromLuaT : public cf::TypeSys::VisitorT
        {
            public:

            VarVisitorSetFromLuaT(lua_State* LuaState);

            void visit(cf::TypeSys::VarT<float>& Var);
            void visit(cf::TypeSys::VarT<double>& Var);
            void visit(cf::TypeSys::VarT<int>& Var);
            void visit(cf::TypeSys::VarT<unsigned int>& Var);
            void visit(cf::TypeSys::VarT<std::string>& Var);
            void visit(cf::TypeSys::VarT<Vector3fT>& Var);
            void visit(cf::TypeSys::VarT< ArrayT<std::string> >& Var);


            private:

            lua_State* m_LuaState;
        };


        /// This visitor writes the value of the visited variable into the given std::ostream,
        /// formatted as Lua code.
        class VarVisitorToLuaCodeT : public cf::TypeSys::VisitorConstT
        {
            public:

            VarVisitorToLuaCodeT(std::ostream& Out);

            void visit(const cf::TypeSys::VarT<float>& Var);
            void visit(const cf::TypeSys::VarT<double>& Var);
            void visit(const cf::TypeSys::VarT<int>& Var);
            void visit(const cf::TypeSys::VarT<unsigned int>& Var);
            void visit(const cf::TypeSys::VarT<std::string>& Var);
            void visit(const cf::TypeSys::VarT<Vector3fT>& Var);
            void visit(const cf::TypeSys::VarT< ArrayT<std::string> >& Var);


            private:

            void WriteString(const std::string& s) const;

            std::ostream& m_Out;
        };
    }
}

#endif
