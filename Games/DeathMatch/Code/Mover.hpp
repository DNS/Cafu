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

#ifndef _MOVER_HPP_
#define _MOVER_HPP_

#include "../../BaseEntity.hpp"


class EntityCreateParamsT;
struct luaL_Reg;
namespace cf { namespace SceneGraph { class GenericNodeT; } }


class EntFuncMoverT : public BaseEntityT
{
    public:

    EntFuncMoverT(const EntityCreateParamsT& Params);
    void Think(float FrameTime, unsigned long ServerFrameNr);
    void Cl_UnserializeFrom();
    void Draw(bool FirstPersonView, float LodDist) const;


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    Vector3dT TranslationSource;
    Vector3dT TranslationDest;
    float     TranslationLinTimeTotal;
    float     TranslationLinTimeLeft;

    const cf::SceneGraph::GenericNodeT* RootNode;   ///< The root node of the scene graph of the model (brushwork) of this entity.


    // Script methods (to be called from the map/entity Lua scripts).
    static int SetOrigin(lua_State* LuaState);  // Override for the base class method (we also have to update our ClipModel origin).
    static int Translate(lua_State* LuaState);  // Translates this entity to a given position over a given time, with acceleration an deceleration.
    static int Rotate   (lua_State* LuaState);  // Rotates this entity.

    static const luaL_Reg MethodsList[];
};

#endif
