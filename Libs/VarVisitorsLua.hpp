/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_TYPESYS_VAR_VISITORS_LUA_HPP_INCLUDED
#define CAFU_TYPESYS_VAR_VISITORS_LUA_HPP_INCLUDED

#include "Variables.hpp"


struct lua_State;


namespace cf
{
    namespace TypeSys
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
            void visit(const cf::TypeSys::VarT<uint16_t>& Var);
            void visit(const cf::TypeSys::VarT<uint8_t>& Var);
            void visit(const cf::TypeSys::VarT<bool>& Var);
            void visit(const cf::TypeSys::VarT<std::string>& Var);
            void visit(const cf::TypeSys::VarT<Vector2fT>& Var);
            void visit(const cf::TypeSys::VarT<Vector3fT>& Var);
            void visit(const cf::TypeSys::VarT<Vector3dT>& Var);
            void visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var);
            void visit(const cf::TypeSys::VarArrayT<uint32_t>& Var);
            void visit(const cf::TypeSys::VarArrayT<uint16_t>& Var);
            void visit(const cf::TypeSys::VarArrayT<uint8_t>& Var);
            void visit(const cf::TypeSys::VarArrayT<std::string>& Var);


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
            void visit(cf::TypeSys::VarT<uint16_t>& Var);
            void visit(cf::TypeSys::VarT<uint8_t>& Var);
            void visit(cf::TypeSys::VarT<bool>& Var);
            void visit(cf::TypeSys::VarT<std::string>& Var);
            void visit(cf::TypeSys::VarT<Vector2fT>& Var);
            void visit(cf::TypeSys::VarT<Vector3fT>& Var);
            void visit(cf::TypeSys::VarT<Vector3dT>& Var);
            void visit(cf::TypeSys::VarT<BoundingBox3dT>& Var);
            void visit(cf::TypeSys::VarArrayT<uint32_t>& Var);
            void visit(cf::TypeSys::VarArrayT<uint16_t>& Var);
            void visit(cf::TypeSys::VarArrayT<uint8_t>& Var);
            void visit(cf::TypeSys::VarArrayT<std::string>& Var);


            private:

            lua_State* m_LuaState;
        };


        /// This visitor is used to set `float` values in variables that are of type `float`, or composed of `float`.
        class VarVisitorSetFloatT : public cf::TypeSys::VisitorT
        {
            public:

            VarVisitorSetFloatT(unsigned int Suffix, float Value);

            void visit(cf::TypeSys::VarT<float>& Var);
            void visit(cf::TypeSys::VarT<double>& Var);
            void visit(cf::TypeSys::VarT<int>& Var);
            void visit(cf::TypeSys::VarT<unsigned int>& Var);
            void visit(cf::TypeSys::VarT<uint16_t>& Var);
            void visit(cf::TypeSys::VarT<uint8_t>& Var);
            void visit(cf::TypeSys::VarT<bool>& Var);
            void visit(cf::TypeSys::VarT<std::string>& Var);
            void visit(cf::TypeSys::VarT<Vector2fT>& Var);
            void visit(cf::TypeSys::VarT<Vector3fT>& Var);
            void visit(cf::TypeSys::VarT<Vector3dT>& Var);
            void visit(cf::TypeSys::VarT<BoundingBox3dT>& Var);
            void visit(cf::TypeSys::VarArrayT<uint32_t>& Var);
            void visit(cf::TypeSys::VarArrayT<uint16_t>& Var);
            void visit(cf::TypeSys::VarArrayT<uint8_t>& Var);
            void visit(cf::TypeSys::VarArrayT<std::string>& Var);


            private:

            unsigned int m_Suffix;
            float        m_Value;
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
            void visit(const cf::TypeSys::VarT<uint16_t>& Var);
            void visit(const cf::TypeSys::VarT<uint8_t>& Var);
            void visit(const cf::TypeSys::VarT<bool>& Var);
            void visit(const cf::TypeSys::VarT<std::string>& Var);
            void visit(const cf::TypeSys::VarT<Vector2fT>& Var);
            void visit(const cf::TypeSys::VarT<Vector3fT>& Var);
            void visit(const cf::TypeSys::VarT<Vector3dT>& Var);
            void visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var);
            void visit(const cf::TypeSys::VarArrayT<uint32_t>& Var);
            void visit(const cf::TypeSys::VarArrayT<uint16_t>& Var);
            void visit(const cf::TypeSys::VarArrayT<uint8_t>& Var);
            void visit(const cf::TypeSys::VarArrayT<std::string>& Var);


            private:

            void WriteString(const std::string& s) const;

            std::ostream& m_Out;
        };
    }
}

#endif
