/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "VarVisitorsLua.hpp"

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
}

#include <cctype>


using namespace cf::TypeSys;


/***************************/
/*** VarVisitorGetToLuaT ***/
/***************************/

VarVisitorGetToLuaT::VarVisitorGetToLuaT(lua_State* LuaState)
    : m_LuaState(LuaState),
      m_NumResults(0)
{
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<float>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<double>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<int>& Var)
{
    lua_pushinteger(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<unsigned int>& Var)
{
    lua_pushinteger(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<uint16_t>& Var)
{
    lua_pushinteger(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<uint8_t>& Var)
{
    lua_pushinteger(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<bool>& Var)
{
    lua_pushboolean(m_LuaState, Var.Get());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<std::string>& Var)
{
    lua_pushstring(m_LuaState, Var.Get().c_str());
    m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<Vector2fT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().y); m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().z); m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<Vector3dT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().z); m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    lua_pushnumber(m_LuaState, Var.Get().Min.x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().Min.y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().Min.z); m_NumResults++;

    lua_pushnumber(m_LuaState, Var.Get().Max.x); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().Max.y); m_NumResults++;
    lua_pushnumber(m_LuaState, Var.Get().Max.z); m_NumResults++;
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        lua_pushinteger(m_LuaState, Var[i]);
        lua_rawseti(m_LuaState, -2, i + 1);   // Lua array numbering starts per convention at 1.
    }
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        lua_pushinteger(m_LuaState, Var[i]);
        lua_rawseti(m_LuaState, -2, i + 1);   // Lua array numbering starts per convention at 1.
    }
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        lua_pushinteger(m_LuaState, Var[i]);
        lua_rawseti(m_LuaState, -2, i + 1);   // Lua array numbering starts per convention at 1.
    }
}


void VarVisitorGetToLuaT::visit(const cf::TypeSys::VarArrayT<std::string>& Var)
{
    lua_newtable(m_LuaState);
    m_NumResults++;

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        lua_pushstring(m_LuaState, Var[i].c_str());
        lua_rawseti(m_LuaState, -2, i + 1);   // Lua array numbering starts per convention at 1.
    }
}


/*****************************/
/*** VarVisitorSetFromLuaT ***/
/*****************************/

VarVisitorSetFromLuaT::VarVisitorSetFromLuaT(lua_State* LuaState)
    : m_LuaState(LuaState)
{
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<float>& Var)
{
    Var.Set(float(luaL_checknumber(m_LuaState, -1)));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<double>& Var)
{
    Var.Set(luaL_checknumber(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<int>& Var)
{
    Var.Set(luaL_checkint(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<unsigned int>& Var)
{
    Var.Set(luaL_checkint(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<uint16_t>& Var)
{
    Var.Set(luaL_checkint(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<uint8_t>& Var)
{
    Var.Set(luaL_checkint(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<bool>& Var)
{
    // Also treat the number 0 as false, not just "false" and "nil".
    if (lua_isnumber(m_LuaState, -1))
        Var.Set(lua_tonumber(m_LuaState, -1) != 0.0);
    else
        Var.Set(lua_toboolean(m_LuaState, -1) != 0);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<std::string>& Var)
{
    Var.Set(luaL_checkstring(m_LuaState, -1));
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    Vector2fT v;

    if (lua_istable(m_LuaState, -1))
    {
        lua_rawgeti(m_LuaState, -1, 1); v.x = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 2); v.y = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
    }
    else
    {
        v.x = float(luaL_checknumber(m_LuaState, -2));
        v.y = float(luaL_checknumber(m_LuaState, -1));
    }

    Var.Set(v);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT v;

    if (lua_istable(m_LuaState, -1))
    {
        lua_rawgeti(m_LuaState, -1, 1); v.x = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 2); v.y = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 3); v.z = float(lua_tonumber(m_LuaState, -1)); lua_pop(m_LuaState, 1);
    }
    else
    {
        v.x = float(luaL_checknumber(m_LuaState, -3));
        v.y = float(luaL_checknumber(m_LuaState, -2));
        v.z = float(luaL_checknumber(m_LuaState, -1));
    }

    Var.Set(v);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<Vector3dT>& Var)
{
    Vector3dT v;

    if (lua_istable(m_LuaState, -1))
    {
        lua_rawgeti(m_LuaState, -1, 1); v.x = lua_tonumber(m_LuaState, -1); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 2); v.y = lua_tonumber(m_LuaState, -1); lua_pop(m_LuaState, 1);
        lua_rawgeti(m_LuaState, -1, 3); v.z = lua_tonumber(m_LuaState, -1); lua_pop(m_LuaState, 1);
    }
    else
    {
        v.x = luaL_checknumber(m_LuaState, -3);
        v.y = luaL_checknumber(m_LuaState, -2);
        v.z = luaL_checknumber(m_LuaState, -1);
    }

    Var.Set(v);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    BoundingBox3dT BB;

    BB.Min.x = luaL_checknumber(m_LuaState, -6);
    BB.Min.y = luaL_checknumber(m_LuaState, -5);
    BB.Min.z = luaL_checknumber(m_LuaState, -4);

    BB.Max.x = luaL_checknumber(m_LuaState, -3);
    BB.Max.y = luaL_checknumber(m_LuaState, -2);
    BB.Max.z = luaL_checknumber(m_LuaState, -1);

    Var.Set(BB);
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    Var.Overwrite();

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = int(lua_rawlen(m_LuaState, -1));

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            Var.PushBack(lua_tointeger(m_LuaState, -1));
            lua_pop(m_LuaState, 1);
        }
    }
    else
    {
        // Stack index 1 has the "this" object,
        // stack index 2 has the variable name.
        for (int i = 3; i <= lua_gettop(m_LuaState); i++)
            Var.PushBack(lua_tointeger(m_LuaState, i));
    }
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    Var.Overwrite();

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = int(lua_rawlen(m_LuaState, -1));

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            Var.PushBack(lua_tointeger(m_LuaState, -1));
            lua_pop(m_LuaState, 1);
        }
    }
    else
    {
        // Stack index 1 has the "this" object,
        // stack index 2 has the variable name.
        for (int i = 3; i <= lua_gettop(m_LuaState); i++)
            Var.PushBack(lua_tointeger(m_LuaState, i));
    }
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    Var.Overwrite();

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = int(lua_rawlen(m_LuaState, -1));

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            Var.PushBack(lua_tointeger(m_LuaState, -1));
            lua_pop(m_LuaState, 1);
        }
    }
    else
    {
        // Stack index 1 has the "this" object,
        // stack index 2 has the variable name.
        for (int i = 3; i <= lua_gettop(m_LuaState); i++)
            Var.PushBack(lua_tointeger(m_LuaState, i));
    }
}


void VarVisitorSetFromLuaT::visit(cf::TypeSys::VarArrayT<std::string>& Var)
{
    Var.Overwrite();

    if (lua_istable(m_LuaState, -1))
    {
        const int Num = int(lua_rawlen(m_LuaState, -1));

        for (int i = 1; i <= Num; i++)
        {
            lua_rawgeti(m_LuaState, -1, i);
            const char* s = lua_tostring(m_LuaState, -1);
            Var.PushBack(s ? s : "NULL");
            lua_pop(m_LuaState, 1);
        }
    }
    else
    {
        // Stack index 1 has the "this" object,
        // stack index 2 has the variable name.
        for (int i = 3; i <= lua_gettop(m_LuaState); i++)
        {
            const char* s = lua_tostring(m_LuaState, i);
            Var.PushBack(s ? s : "NULL");
        }
    }
}


/***************************/
/*** VarVisitorSetFloatT ***/
/***************************/

VarVisitorSetFloatT::VarVisitorSetFloatT(unsigned int Suffix, float Value)
    : m_Suffix(Suffix),
      m_Value(Value)
{
}


void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<float>& Var)
{
    Var.Set(m_Value);
}


void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<Vector2fT>& Var)
{
    Vector2fT v = Var.Get();

    v[m_Suffix % 2] = m_Value;

    Var.Set(v);
}


void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<Vector3fT>& Var)
{
    Vector3fT v = Var.Get();

    v[m_Suffix % 3] = m_Value;

    Var.Set(v);
}


void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<double>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<int>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<unsigned int>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<uint16_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<uint8_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<bool>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<std::string>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<Vector3dT>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarT<BoundingBox3dT>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarArrayT<uint32_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarArrayT<uint16_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarArrayT<uint8_t>& Var) { }
void VarVisitorSetFloatT::visit(cf::TypeSys::VarArrayT<std::string>& Var) { }


/****************************/
/*** VarVisitorToLuaCodeT ***/
/****************************/

namespace
{
    /// This function serializes a given float f1 to a string s, such that:
    ///   - s is minimal (uses the least number of decimal digits required),
    ///   - unserializing s back to a float f2 yields f1 == f2.
    /// See my post "float to string to float, with first float == second float"
    /// to comp.lang.c++ on 2009-10-06 for additional details.
    template<class T> std::string serialize(const T f1)
    {
        // Make sure that if f1 is -0, "0" instead of "-0" is returned.
        if (f1 == 0.0f) return "0";

        // From MSDN documentation: "digits10 returns the number of decimal digits that the type can represent without loss of precision."
        // For floats, that's usually 6, for doubles, that's usually 15. However, we want to use the number of *significant* decimal digits here,
        // that is, max_digits10. See http://www.open-std.org/JTC1/sc22/wg21/docs/papers/2006/n2005.pdf for details.
        const unsigned int DIGITS10     = std::numeric_limits<T>::digits10;
        const unsigned int MAX_DIGITS10 = DIGITS10 + 3;

        std::string  s;
        unsigned int prec;

        for (prec = DIGITS10; prec <= MAX_DIGITS10; prec++)
        {
            std::stringstream ss;

            ss.precision(prec);
            ss << f1;

            s = ss.str();

#if defined(_MSC_VER) && (_MSC_VER <= 1900)     // 1900 == Visual C++ 14.0 (2015)
            // There is a bug in Microsoft's iostream implementation up to Visual C++ 2015,
            // see http://trac.cafu.de/ticket/150 for details.
            const T f2 = T(atof(s.c_str()));
#else
            T f2;
            ss >> f2;
#endif

            if (f2 == f1) break;
        }

        assert(prec <= MAX_DIGITS10);
        return s;
    }
}


VarVisitorToLuaCodeT::VarVisitorToLuaCodeT(std::ostream& Out)
    : m_Out(Out)
{
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<float>& Var)
{
    m_Out << serialize(Var.Get());
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<double>& Var)
{
    m_Out << serialize(Var.Get());
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<int>& Var)
{
    m_Out << Var.Get();
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<unsigned int>& Var)
{
    m_Out << Var.Get();
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<uint16_t>& Var)
{
    m_Out << Var.Get();
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<uint8_t>& Var)
{
    m_Out << uint16_t(Var.Get());   // Write numbers, not characters.
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<bool>& Var)
{
    m_Out << (Var.Get() ? "true" : "false");
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<std::string>& Var)
{
    WriteString(Var.Get());
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<Vector2fT>& Var)
{
    m_Out << serialize(Var.Get().x) << ", " << serialize(Var.Get().y);
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<Vector3fT>& Var)
{
    m_Out << serialize(Var.Get().x) << ", " << serialize(Var.Get().y) << ", " << serialize(Var.Get().z);
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<Vector3dT>& Var)
{
    m_Out << serialize(Var.Get().x) << ", " << serialize(Var.Get().y) << ", " << serialize(Var.Get().z);
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarT<BoundingBox3dT>& Var)
{
    m_Out << serialize(Var.Get().Min.x) << ", " << serialize(Var.Get().Min.y) << ", " << serialize(Var.Get().Min.z) << ", ";
    m_Out << serialize(Var.Get().Max.x) << ", " << serialize(Var.Get().Max.y) << ", " << serialize(Var.Get().Max.z);
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarArrayT<uint32_t>& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        m_Out << Var[i];
        if (i+1 < Var.Size()) m_Out << ", ";
    }

    m_Out << " }";
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarArrayT<uint16_t>& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        m_Out << Var[i];
        if (i+1 < Var.Size()) m_Out << ", ";
    }

    m_Out << " }";
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarArrayT<uint8_t>& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        m_Out << uint16_t(Var[i]);    // Write numbers, not characters.
        if (i+1 < Var.Size()) m_Out << ", ";
    }

    m_Out << " }";
}


void VarVisitorToLuaCodeT::visit(const cf::TypeSys::VarArrayT<std::string>& Var)
{
    m_Out << "{ ";

    for (unsigned int i = 0; i < Var.Size(); i++)
    {
        WriteString(Var[i]);
        if (i+1 < Var.Size()) m_Out << ", ";
    }

    m_Out << " }";
}


void VarVisitorToLuaCodeT::WriteString(const std::string& s) const
{
    for (size_t i = 0; i < s.size(); i++)
        if (iscntrl(s[i]) || s[i] == '"' || s[i] == '\\')
        {
            std::string Equals = "";

            while (s.find("[" + Equals + "[") != std::string::npos ||
                   s.find("]" + Equals + "]") != std::string::npos)
                Equals += "=";

            // Why do we write an extra newline following the opening long bracket?
            // The answer is given in the Lua reference manual:
            //
            // > For convenience, when the opening long bracket is immediately
            // > followed by a newline, the newline is not included in the string.
            //
            // That is, if s begins with a character that is *not* a newline, prepending the extra newline
            // doesn't make a difference. But if the first character in s happened to be a newline, it would
            // get lost if the extra newline was not written.
            m_Out << "[" << Equals << "[\n";
            m_Out << s;
            m_Out << "]" << Equals << "]";
            return;
        }

    m_Out << "\"" << s << "\"";
}
