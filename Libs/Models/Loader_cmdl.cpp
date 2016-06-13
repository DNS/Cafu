/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Loader_cmdl.hpp"
#include "ConsoleCommands/Console.hpp"      // for cf::va()
#include "MaterialSystem/Material.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


LoaderCafuT::LoaderCafuT(const std::string& FileName, int Flags)
    : ModelLoaderT(FileName, Flags),
      m_LuaState(luaL_newstate()),
      m_Version(0)
{
    if (!m_LuaState) throw LoadErrorT("Couldn't open Lua state.");

    luaL_requiref(m_LuaState, "_G",            luaopen_base,      1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_LOADLIBNAME, luaopen_package,   1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_COLIBNAME,   luaopen_coroutine, 1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_TABLIBNAME,  luaopen_table,     1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_IOLIBNAME,   luaopen_io,        1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_OSLIBNAME,   luaopen_os,        1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_STRLIBNAME,  luaopen_string,    1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_BITLIBNAME,  luaopen_bit32,     1); lua_pop(m_LuaState, 1);
    luaL_requiref(m_LuaState, LUA_MATHLIBNAME, luaopen_math,      1); lua_pop(m_LuaState, 1);

    // Set  REGISTRY["LoaderCafuT"]=this  so that our custom functions can obtain the pointer to this LoaderCafuT instance.
    lua_pushlightuserdata(m_LuaState, this);
    lua_setfield(m_LuaState, LUA_REGISTRYINDEX, "LoaderCafuT");

    // Add our custom global functions to the Lua state.
    lua_pushcfunction(m_LuaState, SetVersion);
    lua_setglobal(m_LuaState, "Version");

    // Load and process the Lua program that defines the model.
    if (luaL_loadfile(m_LuaState, m_FileName.c_str())!=0 || lua_pcall(m_LuaState, 0, 0, 0)!=0)
    {
        const std::string LuaError=lua_tostring(m_LuaState, -1);
        lua_close(m_LuaState);
        throw LoadErrorT(LuaError);
    }

#if 0
    // [No need for global model properties at this time.]
    // Read the global properties.
    lua_getglobal(m_LuaState, "Properties");
    {
        lua_getfield(m_LuaState, -1, "useGivenTS");
        m_UseGivenTS=lua_isnumber(m_LuaState, -1) ? (lua_tonumber(m_LuaState, -1)!=0) : (lua_toboolean(m_LuaState, -1)!=0);
        lua_pop(m_LuaState, 1);
    }
    lua_pop(m_LuaState, 1);
#endif
}


LoaderCafuT::~LoaderCafuT()
{
    lua_close(m_LuaState);
}


/// Reads a Vector3fT from the table at the given stack index.
static Vector3fT ReadVector3f(lua_State* LuaState, int index)
{
    Vector3fT v;

    for (unsigned int c=0; c<3; c++)
    {
        lua_rawgeti(LuaState, index, c+1);
        v[c]=float(lua_tonumber(LuaState, -1));
        lua_pop(LuaState, 1);
    }

    return v;
}


static unsigned long lua_objlen_ul(lua_State* LuaState, int index)
{
    const size_t l = lua_rawlen(LuaState, index);

    return (unsigned long)l;
}


void LoaderCafuT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
    // Check the file format version.
    if (m_Version<1 || m_Version>CafuModelT::CMDL_FILE_VERSION)
    {
                             // "Expected cmdl file format version 1 to %u, ..."
        throw LoadErrorT(cf::va("Expected cmdl file format version %u, but found version %u.", CafuModelT::CMDL_FILE_VERSION, m_Version));
    }


    // Read the joints.
    lua_getglobal(m_LuaState, "Joints");
    {
        Joints.Overwrite();
        Joints.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

        for (unsigned long JointNr=0; JointNr<Joints.Size(); JointNr++)
        {
            CafuModelT::JointT& Joint=Joints[JointNr];

            lua_rawgeti(m_LuaState, -1, JointNr+1);
            {
                lua_getfield(m_LuaState, -1, "name");
                const char* Name=lua_tostring(m_LuaState, -1);
                Joint.Name=Name ? Name : "Joint";
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "parent");
                Joint.Parent=lua_tointeger(m_LuaState, -1);
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "pos");
                Joint.Pos=ReadVector3f(m_LuaState, -1);
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "qtr");
                Joint.Qtr=ReadVector3f(m_LuaState, -1);
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "scale");
                Joint.Scale=ReadVector3f(m_LuaState, -1);
                lua_pop(m_LuaState, 1);
            }
            lua_pop(m_LuaState, 1);
        }
    }
    lua_pop(m_LuaState, 1);


    // Read the meshes.
    lua_getglobal(m_LuaState, "Meshes");
    {
        Meshes.Overwrite();
        Meshes.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

        for (unsigned long MeshNr=0; MeshNr<Meshes.Size(); MeshNr++)
        {
            CafuModelT::MeshT& Mesh=Meshes[MeshNr];

            lua_rawgeti(m_LuaState, -1, MeshNr+1);
            {
                lua_getfield(m_LuaState, -1, "name");
                {
                    const char* Name=lua_tostring(m_LuaState, -1);
                    Mesh.Name=Name ? Name : "Mesh";
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "Material");
                {
                    const char*       s=lua_tostring(m_LuaState, -1);
                    const std::string MatName=s ? s : "wire-frame";

                    Mesh.Material=MaterialMan.GetMaterial(MatName);

                    if (!Mesh.Material)
                    {
                        // Our .cmdl model materials really depend on the related .cmat file.
                        // Thus if a material cannot be found in MaterialMan, go for the wire-frame substitute straight away.
                        Mesh.Material=MaterialMan.RegisterMaterial(CreateDefaultMaterial(MatName));
                    }
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "tsMethod");
                {
                    const char* Method=lua_tostring(m_LuaState, -1);
                    Mesh.SetTSMethod(Method ? Method : "default");
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "castShadows");
                {
                    Mesh.CastShadows=lua_isnumber(m_LuaState, -1) ? (lua_tonumber(m_LuaState, -1)!=0) : (lua_toboolean(m_LuaState, -1)!=0);
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "Weights");
                {
                    Mesh.Weights.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

                    for (unsigned long WeightNr=0; WeightNr<Mesh.Weights.Size(); WeightNr++)
                    {
                        CafuModelT::MeshT::WeightT& Weight=Mesh.Weights[WeightNr];

                        lua_rawgeti(m_LuaState, -1, WeightNr+1);
                        {
                            lua_getfield(m_LuaState, -1, "joint");
                            Weight.JointIdx=lua_tointeger(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "weight");
                            Weight.Weight=float(lua_tonumber(m_LuaState, -1));
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "pos");
                            Weight.Pos=ReadVector3f(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);
                        }
                        lua_pop(m_LuaState, 1);
                    }
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "Vertices");
                {
                    Mesh.Vertices.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

                    for (unsigned long VertexNr=0; VertexNr<Mesh.Vertices.Size(); VertexNr++)
                    {
                        CafuModelT::MeshT::VertexT& Vertex=Mesh.Vertices[VertexNr];

                        lua_rawgeti(m_LuaState, -1, VertexNr+1);
                        {
                            lua_getfield(m_LuaState, -1, "firstWeight");
                            Vertex.FirstWeightIdx=lua_tointeger(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "numWeights");
                            Vertex.NumWeights=lua_tointeger(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "uv");
                            const Vector3fT uv=ReadVector3f(m_LuaState, -1);
                            Vertex.u=uv.x;
                            Vertex.v=uv.y;
                            lua_pop(m_LuaState, 1);
                        }
                        lua_pop(m_LuaState, 1);
                    }
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "Triangles");
                {
                    Mesh.Triangles.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

                    for (unsigned long TriangleNr=0; TriangleNr<Mesh.Triangles.Size(); TriangleNr++)
                    {
                        CafuModelT::MeshT::TriangleT& Triangle=Mesh.Triangles[TriangleNr];

                        lua_rawgeti(m_LuaState, -1, TriangleNr+1);
                        {
                            for (unsigned int c=0; c<3; c++)
                            {
                                lua_rawgeti(m_LuaState, -1, c+1);
                                Triangle.VertexIdx[c]=lua_tointeger(m_LuaState, -1);
                                lua_pop(m_LuaState, 1);
                            }

                            lua_getfield(m_LuaState, -1, "sg");
                            Triangle.SmoothGroups=uint32_t(lua_tonumber(m_LuaState, -1));
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "skipDraw");
                            Triangle.SkipDraw=lua_toboolean(m_LuaState, -1)!=0;
                            lua_pop(m_LuaState, 1);
                        }
                        lua_pop(m_LuaState, 1);
                    }
                }
                lua_pop(m_LuaState, 1);
            }
            lua_pop(m_LuaState, 1);
        }
    }
    lua_pop(m_LuaState, 1);


    // Read the animations.
    lua_getglobal(m_LuaState, "Animations");
    {
        Anims.Overwrite();
        Anims.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

        for (unsigned long AnimNr=0; AnimNr<Anims.Size(); AnimNr++)
        {
            CafuModelT::AnimT& Anim=Anims[AnimNr];

            lua_rawgeti(m_LuaState, -1, AnimNr+1);
            {
                lua_getfield(m_LuaState, -1, "name");
                {
                    const char* Name=lua_tostring(m_LuaState, -1);
                    Anim.Name=Name ? Name : "Anim";
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "FPS");
                Anim.FPS=float(lua_tonumber(m_LuaState, -1));
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "next");
                Anim.Next=lua_tointeger(m_LuaState, -1);
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "AnimJoints");
                {
                    Anim.AnimJoints.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

                    for (unsigned long JointNr=0; JointNr<Anim.AnimJoints.Size(); JointNr++)
                    {
                        CafuModelT::AnimT::AnimJointT& AnimJoint=Anim.AnimJoints[JointNr];

                        lua_rawgeti(m_LuaState, -1, JointNr+1);
                        {
                            lua_getfield(m_LuaState, -1, "pos");
                            AnimJoint.DefaultPos=ReadVector3f(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "qtr");
                            AnimJoint.DefaultQtr=ReadVector3f(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "scale");
                            AnimJoint.DefaultScale=ReadVector3f(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "flags");
                            AnimJoint.Flags=lua_tointeger(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "firstData");
                            AnimJoint.FirstDataIdx=lua_tointeger(m_LuaState, -1);
                            lua_pop(m_LuaState, 1);
                        }
                        lua_pop(m_LuaState, 1);
                    }
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "Frames");
                {
                    Anim.Frames.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

                    for (unsigned long FrameNr=0; FrameNr<Anim.Frames.Size(); FrameNr++)
                    {
                        CafuModelT::AnimT::FrameT& Frame=Anim.Frames[FrameNr];

                        lua_rawgeti(m_LuaState, -1, FrameNr+1);
                        {
                            lua_getfield(m_LuaState, -1, "bb");
                            {
                                for (unsigned int c=0; c<6; c++)
                                {
                                    lua_rawgeti(m_LuaState, -1, c+1);
                                    if (c<3) Frame.BB.Min[c  ]=float(lua_tonumber(m_LuaState, -1));
                                        else Frame.BB.Max[c-3]=float(lua_tonumber(m_LuaState, -1));
                                    lua_pop(m_LuaState, 1);
                                }
                            }
                            lua_pop(m_LuaState, 1);

                            lua_getfield(m_LuaState, -1, "data");
                            {
                                Frame.AnimData.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

                                for (unsigned int c=0; c<Frame.AnimData.Size(); c++)
                                {
                                    lua_rawgeti(m_LuaState, -1, c+1);
                                    Frame.AnimData[c]=float(lua_tonumber(m_LuaState, -1));
                                    lua_pop(m_LuaState, 1);
                                }
                            }
                            lua_pop(m_LuaState, 1);
                        }
                        lua_pop(m_LuaState, 1);
                    }
                }
                lua_pop(m_LuaState, 1);
            }
            lua_pop(m_LuaState, 1);
        }
    }
    lua_pop(m_LuaState, 1);
}


void LoaderCafuT::Load(ArrayT<CafuModelT::SkinT>& Skins, const MaterialManagerImplT& MaterialMan)
{
    // Read the skins.
    lua_getglobal(m_LuaState, "Skins");
    {
        Skins.Overwrite();
        Skins.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

        for (unsigned long SkinNr=0; SkinNr<Skins.Size(); SkinNr++)
        {
            CafuModelT::SkinT& Skin=Skins[SkinNr];

            lua_rawgeti(m_LuaState, -1, SkinNr+1);
            {
                lua_getfield(m_LuaState, -1, "name");
                {
                    const char* Name=lua_tostring(m_LuaState, -1);
                    Skin.Name=Name ? Name : "Skin";
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "materials");
                {
                    const unsigned long NumMats=lua_objlen_ul(m_LuaState, -1);

                    Skin.Materials.PushBackEmptyExact(NumMats);
                    Skin.RenderMaterials.PushBackEmptyExact(NumMats);

                    for (unsigned int c=0; c<NumMats; c++)
                    {
                        lua_rawgeti(m_LuaState, -1, c+1);
                        {
                            const char*       s=lua_tostring(m_LuaState, -1);
                            const std::string MatName=s ? s : "";

                            Skin.Materials[c]      =(MatName!="") ? MaterialMan.GetMaterial(MatName) : NULL;
                            Skin.RenderMaterials[c]=NULL;
                        }
                        lua_pop(m_LuaState, 1);
                    }
                }
                lua_pop(m_LuaState, 1);
            }
            lua_pop(m_LuaState, 1);
        }
    }
    lua_pop(m_LuaState, 1);
}


void LoaderCafuT::Load(ArrayT<CafuModelT::GuiFixtureT>& GuiFixtures)
{
    // Read the GUI fixtures.
    lua_getglobal(m_LuaState, "GuiFixtures");
    {
        GuiFixtures.Overwrite();
        GuiFixtures.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

        for (unsigned long FixNr=0; FixNr<GuiFixtures.Size(); FixNr++)
        {
            CafuModelT::GuiFixtureT& GuiFixture=GuiFixtures[FixNr];

            lua_rawgeti(m_LuaState, -1, FixNr+1);
            {
                lua_getfield(m_LuaState, -1, "name");
                {
                    const char* Name=lua_tostring(m_LuaState, -1);
                    GuiFixture.Name=Name ? Name : "GUI Fixture";
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "points");
                for (unsigned int c=0; c<6; c++)
                {
                    lua_rawgeti(m_LuaState, -1, c+1);
                    if ((c % 2)==0) GuiFixture.Points[c/2].MeshNr  =lua_tointeger(m_LuaState, -1);
                               else GuiFixture.Points[c/2].VertexNr=lua_tointeger(m_LuaState, -1);
                    lua_pop(m_LuaState, 1);
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "trans");
                for (unsigned int c=0; c<2; c++)
                {
                    lua_rawgeti(m_LuaState, -1, c+1);
                    GuiFixture.Trans[c]=float(lua_tonumber(m_LuaState, -1));
                    lua_pop(m_LuaState, 1);
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "scale");
                for (unsigned int c=0; c<2; c++)
                {
                    lua_rawgeti(m_LuaState, -1, c+1);
                    GuiFixture.Scale[c]=float(lua_tonumber(m_LuaState, -1));
                    lua_pop(m_LuaState, 1);
                }
                lua_pop(m_LuaState, 1);
            }
            lua_pop(m_LuaState, 1);
        }
    }
    lua_pop(m_LuaState, 1);
}


void LoaderCafuT::Load(ArrayT<CafuModelT::ChannelT>& Channels)
{
    // Read the channels.
    lua_getglobal(m_LuaState, "Channels");
    {
        Channels.Overwrite();
        Channels.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

        for (unsigned long ChannelNr=0; ChannelNr<Channels.Size(); ChannelNr++)
        {
            CafuModelT::ChannelT& Channel=Channels[ChannelNr];

            lua_rawgeti(m_LuaState, -1, ChannelNr+1);
            {
                lua_getfield(m_LuaState, -1, "name");
                {
                    const char* Name=lua_tostring(m_LuaState, -1);
                    Channel.Name=Name ? Name : "Channel";
                }
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "joints");
                {
                    const unsigned long NumJoints=lua_objlen_ul(m_LuaState, -1);

                    for (unsigned int c=0; c<NumJoints; c++)
                    {
                        lua_rawgeti(m_LuaState, -1, c+1);
                        Channel.SetMember(lua_tointeger(m_LuaState, -1));
                        lua_pop(m_LuaState, 1);
                    }
                }
                lua_pop(m_LuaState, 1);
            }
            lua_pop(m_LuaState, 1);
        }
    }
    lua_pop(m_LuaState, 1);
}


/*static*/ int LoaderCafuT::SetVersion(lua_State* LuaState)
{
    // Put REGISTRY["LoaderCafuT"] onto the stack.
    lua_getfield(LuaState, LUA_REGISTRYINDEX, "LoaderCafuT");

    LoaderCafuT* Loader=(LoaderCafuT*)lua_touserdata(LuaState, -1);
    if (Loader==NULL) luaL_error(LuaState, "NULL pointer to LoaderCafuT instance.");

    // Remove the light userdata from the stack again, restoring the stack to its original state.
    lua_pop(LuaState, 1);

    const unsigned int v=luaL_checkint(LuaState, 1);
    if (Loader->m_Version!=0 && Loader->m_Version!=v) luaL_error(LuaState, "Attempt to redefine the version number of the file format.");

    Loader->m_Version=v;
    return 0;
}
