/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2011 Carsten Fuchs Software.

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

#include "GameConfig.hpp"
#include "EntityClass.hpp"
#include "FileSys/FileMan.hpp"
#include "wx/fileconf.h"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


GameConfigT::GameConfigT(wxFileConfig& CfgFile, const wxString& Name_, const wxString& ModDir_)
    : Name(Name_),
      ModDir(ModDir_),
      m_MatMan(*this),
      m_MaxMapCoord(8192)
{
    // The next line doesn't work with wx 2.6.4, it always returns the default value 1.0.
 // DefaultTextureScale =CfgFile.Read("DefaultTextureScale", 1.0);
    double d;
    CfgFile.Read("DefaultTextureScale",  &d, 0.25); DefaultTextureScale =d;     // Work-around...
    CfgFile.Read("DefaultLightmapScale", &d, 16.0); DefaultLightmapScale=d;

    DefaultSolidEntity  =CfgFile.Read("DefaultSolidEntity", "func_wall");
    DefaultPointEntity  =CfgFile.Read("DefaultPointEntity", "info_player_start");
    CordonTexture       =CfgFile.Read("CordonTexture", "Textures/generic/_orange");


    // Mount the file systems relevant for this MOD.
    const std::string ModDirSlash(ModDir+"/");

    m_MountedFileSystems.PushBack(cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_LOCAL_PATH, ModDirSlash, ModDirSlash));
    m_MountedFileSystems.PushBack(cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, ModDirSlash+"Textures/TechDemo.zip", ModDirSlash+"Textures/TechDemo/", "Ca3DE"));
    m_MountedFileSystems.PushBack(cf::FileSys::FileMan->MountFileSystem(cf::FileSys::FS_TYPE_ZIP_ARCHIVE, ModDirSlash+"Textures/SkyDomes.zip", ModDirSlash+"Textures/SkyDomes/", "Ca3DE"));


    // Create a new Lua state.
    lua_State* LuaState=lua_open();

    try
    {
        if (LuaState==NULL) throw InitErrorT();

        lua_pushcfunction(LuaState, luaopen_base);    lua_pushstring(LuaState, "");              lua_call(LuaState, 1, 0);  // Opens the basic library.
        lua_pushcfunction(LuaState, luaopen_package); lua_pushstring(LuaState, LUA_LOADLIBNAME); lua_call(LuaState, 1, 0);  // Opens the package library.
        lua_pushcfunction(LuaState, luaopen_table);   lua_pushstring(LuaState, LUA_TABLIBNAME);  lua_call(LuaState, 1, 0);  // Opens the table library.
        lua_pushcfunction(LuaState, luaopen_io);      lua_pushstring(LuaState, LUA_IOLIBNAME);   lua_call(LuaState, 1, 0);  // Opens the I/O library.
        lua_pushcfunction(LuaState, luaopen_os);      lua_pushstring(LuaState, LUA_OSLIBNAME);   lua_call(LuaState, 1, 0);  // Opens the OS library.
        lua_pushcfunction(LuaState, luaopen_string);  lua_pushstring(LuaState, LUA_STRLIBNAME);  lua_call(LuaState, 1, 0);  // Opens the string lib.
        lua_pushcfunction(LuaState, luaopen_math);    lua_pushstring(LuaState, LUA_MATHLIBNAME); lua_call(LuaState, 1, 0);  // Opens the math lib.


        // Load and process the Lua script file with the entity class definitions.
        if (luaL_loadfile(LuaState, (ModDir+"/EntityClassDefs.lua").c_str())!=0 || lua_pcall(LuaState, 0, 0, 0)!=0)
        {
            wxMessageBox("Entity class definitions script could not be processed,\n"+
                         wxString(lua_tostring(LuaState, -1))+".\n",
                         "Error while initializing game configuration \""+Name+"\"", wxOK | wxICON_ERROR);

            lua_pop(LuaState, 1);
            throw InitErrorT();
        }

        assert(lua_gettop(LuaState)==0);


        // Obtain the "Mapsize" information.
        lua_getglobal(LuaState, "Mapsize");

        if (lua_istable(LuaState, -1))
        {
            lua_rawgeti(LuaState, -1, 1);
            int Min=lua_tointeger(LuaState, -1);
            lua_pop(LuaState, 1);

            lua_rawgeti(LuaState, -1, 2);
            int Max=lua_tointeger(LuaState, -1);
            lua_pop(LuaState, 1);

            if (Min!=0 && Max!=0)
            {
                m_MaxMapCoord=std::max(abs(Min), abs(Max));
            }

            // wxMessageBox(wxString::Format("Found the Mapsize table:    %i,   %i", m_MinMapCoord, m_MaxMapCoord));
        }

        assert(lua_gettop(LuaState)==1);
        lua_pop(LuaState, 1);


        // Create an EntityClassT instance for each entity class definition in the "EntityClassDefs" table.
        lua_getglobal(LuaState, "EntityClassDefs");
        lua_pushnil(LuaState);  // The initial key for the traversal.

        while (lua_next(LuaState, -2)!=0)
        {
            // The key is now at stack index -2, the value is at index -1.
            // Note that in general, the warning from the Lua reference documentation applies:
            // "While traversing a table, do not call lua_tolstring() directly on a key, unless you know that the key is actually a string."
            // Fortunately, we know that the key is of type string, and so the called function is free to inspect (but not to modify!)
            // the two topmost stack values.
            if (lua_type(LuaState, -2)!=LUA_TSTRING)
            {
                wxMessageBox("Non-string entity class name in EntityClassDefs table.\n",
                    "Error while initializing game configuration \""+Name+"\"", wxOK | wxICON_ERROR);
                throw InitErrorT();
            }

            try
            {
                m_EntityClasses.PushBack(new EntityClassT(*this, LuaState));
            }
            catch (const EntityClassT::InitErrorT&)
            {
                // For some reason, the entity class could not be created, and our stack is now in an unknown state
                // (our lower three elements should still be intact, but any number of elements may be on top).
                // We could of course try to recover (pop all elements above ours) and continue,
                // but there is still some risk and there was an error with the script that should be looked into anyway,
                // so we're just bailing out.
                throw InitErrorT();
            }

            // Make sure that the EntityClassT ctor left the stack behind properly.
            assert(lua_gettop(LuaState)==3);

            // Remove the value, keep the key for the next iteration.
            lua_pop(LuaState, 1);
        }

        assert(lua_gettop(LuaState)==1);
        lua_pop(LuaState, 1);
    }
    catch (const InitErrorT&)
    {
        // What is done here is of course utterly against all (RAII-related) advice, see
        // a) Bjarne Stroustrup, "Die C++ Programmiersprache", chapter 14.4.
        // b) Scott Meyers, "Effective C++, 3rd Edition", item 13 (and 14).
        // c) http://en.wikipedia.org/wiki/Resource_Acquisition_Is_Initialization
        // Unfortunately, auto_ptr<> is not well suitable here, and I currently see no
        // easy solution rather than to do it like this.

        // Close the LuaState.
        if (LuaState!=NULL) lua_close(LuaState);

        // Delete all entity classes.
        for (unsigned long ECNr=0; ECNr<m_EntityClasses.Size(); ECNr++)
            delete m_EntityClasses[ECNr];

        // Unmount any mounted file systems.
        for (unsigned long MFSNr=0; MFSNr<m_MountedFileSystems.Size(); MFSNr++)
            cf::FileSys::FileMan->Unmount(m_MountedFileSystems[MFSNr]);

        throw;  // Re-throw the caught exception.
    }

    // Close the LuaState.
    lua_close(LuaState);


    // Output a brief overview of the loaded classes and their variables.
    // This is for debugging purposes.
#if 0
    wxString out;

    for (unsigned long ClassNr=0; ClassNr<m_EntityClasses.Size(); ClassNr++)
    {
        const EntityClassT* EC=m_EntityClasses[ClassNr];

        out+=EC->GetName()+": "+EC->GetDescription()+"\n";
        out+=EC->GetColor().GetAsString(wxC2S_CSS_SYNTAX)+", ["+convertToString(EC->GetBoundingBox().Min).c_str()+", "+convertToString(EC->GetBoundingBox().Max).c_str()+"]\n";

        if (EC->IsSolidClass()) out+="isSolid, ";

        out+="Helpers: ";
        for (unsigned long HNr=0; HNr<EC->GetHelpers().Size(); HNr++)
        {
            const HelperInfoT* HI=EC->GetHelpers()[HNr];

            out+=HI->Name+"(";
            for (unsigned long PNr=0; PNr<HI->Parameters.Size(); PNr++) out+=HI->Parameters[PNr]+" ";
            out+=") ";
        }
        out+="\n";

        // out+="Vars:\n";
        for (unsigned long VarNr=0; VarNr<EC->GetVariables().Size(); VarNr++)
        {
            const EntClassVarT* Var=EC->GetVariables()[VarNr];

            out+="           "+Var->GetName()+", "+Var->GetLongName()+", "+Var->GetDescription()+wxString::Format("  Type: %i", Var->GetType())+"\n";
        }

        out+="\n";
    }

    // wxMessageBox(out, "Entity Classes Overview", wxOK | wxICON_INFORMATION);
    wxLogDebug(out);
#endif
}


GameConfigT::~GameConfigT()
{
    // Delete all entity classes.
    for (unsigned long ECNr=0; ECNr<m_EntityClasses.Size(); ECNr++)
        delete m_EntityClasses[ECNr];

    if (cf::FileSys::FileMan!=NULL)
    {
        for (unsigned long MFSNr=0; MFSNr<m_MountedFileSystems.Size(); MFSNr++)
            cf::FileSys::FileMan->Unmount(m_MountedFileSystems[MFSNr]);
    }

    m_MountedFileSystems.Clear();
}


const EntityClassT* GameConfigT::FindClass(const wxString& Name) const
{
    for (unsigned long ClassNr=0; ClassNr<m_EntityClasses.Size(); ClassNr++)
        if (m_EntityClasses[ClassNr]->GetName()==Name)
            return m_EntityClasses[ClassNr];

    return NULL;
}


BoundingBox3fT GameConfigT::GetMaxMapBB() const
{
    const float f=m_MaxMapCoord;

    return BoundingBox3fT(Vector3fT(-f, -f, -f), Vector3fT(f, f, f));
}


void GameConfigT::Save(wxFileConfig& CfgFile) const
{
    CfgFile.DeleteGroup(".");

    CfgFile.Write("DefaultTextureScale",  DefaultTextureScale);
    CfgFile.Write("DefaultLightmapScale", DefaultLightmapScale);

    CfgFile.Write("DefaultSolidEntity", DefaultSolidEntity);
    CfgFile.Write("DefaultPointEntity", DefaultPointEntity);

    CfgFile.Write("CordonTexture", CordonTexture);
}
