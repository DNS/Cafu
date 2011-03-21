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

#include "Loader_cmdl.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}


LoaderCafuT::LoaderCafuT(const std::string& FileName) /*throw (ModelT::LoadError)*/
    : ModelLoaderT(FileName),
      m_LuaState(lua_open()),
      m_UseGivenTS(false)
{
    if (!m_LuaState) throw LoadErrorT("Couldn't open Lua state.");

    lua_pushcfunction(m_LuaState, luaopen_base);    lua_pushstring(m_LuaState, "");              lua_call(m_LuaState, 1, 0);  // Opens the basic library.
    lua_pushcfunction(m_LuaState, luaopen_package); lua_pushstring(m_LuaState, LUA_LOADLIBNAME); lua_call(m_LuaState, 1, 0);  // Opens the package library.
    lua_pushcfunction(m_LuaState, luaopen_table);   lua_pushstring(m_LuaState, LUA_TABLIBNAME);  lua_call(m_LuaState, 1, 0);  // Opens the table library.
    lua_pushcfunction(m_LuaState, luaopen_io);      lua_pushstring(m_LuaState, LUA_IOLIBNAME);   lua_call(m_LuaState, 1, 0);  // Opens the I/O library.
    lua_pushcfunction(m_LuaState, luaopen_os);      lua_pushstring(m_LuaState, LUA_OSLIBNAME);   lua_call(m_LuaState, 1, 0);  // Opens the OS library.
    lua_pushcfunction(m_LuaState, luaopen_string);  lua_pushstring(m_LuaState, LUA_STRLIBNAME);  lua_call(m_LuaState, 1, 0);  // Opens the string lib.
    lua_pushcfunction(m_LuaState, luaopen_math);    lua_pushstring(m_LuaState, LUA_MATHLIBNAME); lua_call(m_LuaState, 1, 0);  // Opens the math lib.

    // Load and process the Lua program that defines the model.
    if (luaL_loadfile(m_LuaState, m_FileName.c_str())!=0 || lua_pcall(m_LuaState, 0, 0, 0)!=0)
    {
        const std::string LuaError=lua_tostring(m_LuaState, -1);
        lua_close(m_LuaState);
        throw LoadErrorT(LuaError);
    }

    // Read the global properties.
    lua_getglobal(m_LuaState, "Properties");
    {
        lua_getfield(m_LuaState, -1, "useGivenTS");
        m_UseGivenTS=lua_isnumber(m_LuaState, -1) ? (lua_tonumber(m_LuaState, -1)!=0) : (lua_toboolean(m_LuaState, -1)!=0);
        lua_pop(m_LuaState, 1);
    }
    lua_pop(m_LuaState, 1);
}


LoaderCafuT::~LoaderCafuT()
{
    lua_close(m_LuaState);
}


bool LoaderCafuT::UseGivenTS() const
{
    // TODO:
    // For md5 models, the tangent space vectors are not specified in the file.
    // Tell the Cafu model to recompute them dynamically from spatial and texture coordinates instead.
    // For cmdl models, ... (?)  we have to save the tangent-space in the file ... TODO!

    return m_UseGivenTS;
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
    const size_t l=lua_objlen(LuaState, index);

    return (unsigned long)l;
}


void LoaderCafuT::Load(ArrayT<CafuModelT::JointT>& Joints, ArrayT<CafuModelT::MeshT>& Meshes, ArrayT<CafuModelT::AnimT>& Anims, MaterialManagerImplT& MaterialMan)
{
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
                Joint.Name=Name ? Name : "<NULL>";
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
                lua_getfield(m_LuaState, -1, "Material");
                const char* MatName=lua_tostring(m_LuaState, -1);
                // TODO: If the material is NULL, we must
                //   - create a new material with whatever data there is in the model file,
                //   - failing that, create and use a substitute material.
                Mesh.Material=MaterialMan.GetMaterial(MatName ? MatName : "<NULL>");
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
                lua_getfield(m_LuaState, -1, "FPS");
                Anim.FPS=float(lua_tonumber(m_LuaState, -1));
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


void LoaderCafuT::Load(ArrayT<CafuModelT::GuiLocT>& GuiLocs)
{
    // Read the GUI locations.
    lua_getglobal(m_LuaState, "GuiLocs");
    {
        GuiLocs.Overwrite();
        GuiLocs.PushBackEmptyExact(lua_objlen_ul(m_LuaState, -1));

        for (unsigned long GuiLocNr=0; GuiLocNr<GuiLocs.Size(); GuiLocNr++)
        {
            CafuModelT::GuiLocT& GuiLoc=GuiLocs[GuiLocNr];

            lua_rawgeti(m_LuaState, -1, GuiLocNr+1);
            {
                lua_getfield(m_LuaState, -1, "Origin");
                GuiLoc.Origin=ReadVector3f(m_LuaState, -1);
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "AxisX");
                GuiLoc.AxisX=ReadVector3f(m_LuaState, -1);
                lua_pop(m_LuaState, 1);

                lua_getfield(m_LuaState, -1, "AxisY");
                GuiLoc.AxisY=ReadVector3f(m_LuaState, -1);
                lua_pop(m_LuaState, 1);
            }
            lua_pop(m_LuaState, 1);
        }
    }
    lua_pop(m_LuaState, 1);
}
