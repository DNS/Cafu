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

#include "Console_Lua.hpp"
#include "Console.hpp"

extern "C"
{
    #include <lua.h>
    #include <lauxlib.h>
}


static int LuaConsole_Print(lua_State* LuaState)
{
    const char* s=lua_tostring(LuaState, 1);

    Console->Print(s!=NULL ? s : "[not a string]\n");
    return 0;
}


static int LuaConsole_DevPrint(lua_State* LuaState)
{
    const char* s=lua_tostring(LuaState, 1);

    Console->DevPrint(s!=NULL ? s : "[not a string]\n");
    return 0;
}


static int LuaConsole_Warning(lua_State* LuaState)
{
    const char* s=lua_tostring(LuaState, 1);

    Console->Warning(s!=NULL ? s : "[not a string]\n");
    return 0;
}


static int LuaConsole_DevWarning(lua_State* LuaState)
{
    const char* s=lua_tostring(LuaState, 1);

    Console->DevWarning(s!=NULL ? s : "[not a string]\n");
    return 0;
}


#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #undef FindWindow
#else
    #include <cstring>
    #include <dirent.h>
#endif

/// Returns an array (a table) with the file and directory entries of the given directory.
/// If the string "f" is passed as a second parameter, only entries of type file are returned,
/// if the string "d" is passed as a second parameter, only entries of type directory are returned,
/// all entries are returned in all other cases.
///
/// TODO: This function should be implemented INDEPENDENTLY from the "Console" interface (i.e. elsewhere where
///       it is a better topical fit), but for now the "Console" interface is the only interface that is included
///       in all of our Lua instances (console interpreter, GUIs, map scripts, ...)!
/// IDEA: Can we make ConVars and ConFuncs directly available not only in the console Lua instance, but in arbitrary
///       many Lua instances?? Then the GUI and map scripts could directly access ConVars and ConFuncs, too...!
///       (And GetDir() would be a regular ConFunc.)
static int GetDir(lua_State* LuaState)
{
    std::string DirName;
    const char* DirFilter=NULL;

    // // Support calling this function both as gui.GetDir(...) as well as gui:GetDir(...).
    // if (lua_istable(LuaState, 1))
    // {
    //     // It's called as gui:GetDir(...).
    //     DirName  =luaL_checkstring(LuaState, 2);
    //     DirFilter=lua_tostring(LuaState, 3);
    // }
    // else
    {
        // It's called as gui.GetDir(...).
        DirName  =luaL_checkstring(LuaState, 1);
        DirFilter=lua_tostring(LuaState, 2);
    }

    lua_newtable(LuaState);

    if (DirName=="") return 1;

#ifdef _WIN32
    WIN32_FIND_DATA FindFileData;
    HANDLE          FindHandle=FindFirstFile((DirName+"\\*").c_str(), &FindFileData);
    int             EntryCount=1;   // Lua array numbering starts per convention at 1.

    if (FindHandle==INVALID_HANDLE_VALUE) return 1;

    do
    {
        if (strcmp(FindFileData.cFileName, "." )==0) continue;
        if (strcmp(FindFileData.cFileName, "..")==0) continue;

        if (DirFilter!=NULL)
        {
            const bool IsDir=(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0;

            if (strcmp(DirFilter, "f")==0 &&  IsDir) continue;
            if (strcmp(DirFilter, "d")==0 && !IsDir) continue;
        }

        lua_pushstring(LuaState, FindFileData.cFileName);
        lua_rawseti(LuaState, -2, EntryCount++);
    }
    while (FindNextFile(FindHandle, &FindFileData)!=0);

    if (GetLastError()!=ERROR_NO_MORE_FILES) Console->Warning("Error in GetDir() while enumerating directory entries.\n");
    FindClose(FindHandle);
#else
    DIR* Dir=opendir(DirName.c_str());
    int  EntryCount=1;  // Lua array numbering starts per convention at 1.

    if (!Dir) return 1;

    for (dirent* DirEnt=readdir(Dir); DirEnt!=NULL; DirEnt=readdir(Dir))
    {
        if (strcmp(DirEnt->d_name, "." )==0) continue;
        if (strcmp(DirEnt->d_name, "..")==0) continue;

        if (DirFilter!=NULL)
        {
            DIR* TempDir=opendir((DirName+"/"+DirEnt->d_name).c_str());
            bool IsDir=(TempDir!=NULL);

            if (TempDir!=NULL) closedir(TempDir);

            if (strcmp(DirFilter, "f")==0 &&  IsDir) continue;
            if (strcmp(DirFilter, "d")==0 && !IsDir) continue;
        }

        // For portability, only the 'd_name' member of a 'dirent' may be accessed.
        lua_pushstring(LuaState, DirEnt->d_name);
        lua_rawseti(LuaState, -2, EntryCount++);
    }

    closedir(Dir);
#endif

    return 1;
}


void cf::Console_RegisterLua(lua_State* LuaState)
{
    static const luaL_Reg ConsoleFunctions[]=
    {
        { "Print",      LuaConsole_Print },
        { "DevPrint",   LuaConsole_DevPrint },
        { "Warning",    LuaConsole_Warning },
        { "DevWarning", LuaConsole_DevWarning },
        { "GetDir",     GetDir },
        { NULL, NULL }
    };

    luaL_register(LuaState, "Console", ConsoleFunctions);
    lua_pop(LuaState, 1);   // Remove the Console table from the stack (it was left there by the luaL_register() function).
}
