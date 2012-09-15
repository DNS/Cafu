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

#ifndef CAFU_FUNC_DOOR_HPP_INCLUDED
#define CAFU_FUNC_DOOR_HPP_INCLUDED

#include "BaseEntity.hpp"


struct luaL_Reg;
namespace cf { namespace SceneGraph { class GenericNodeT; } }


namespace GAME_NAME
{
    class EntFuncDoorT : public BaseEntityT
    {
        public:

        EntFuncDoorT(const EntityCreateParamsT& Params);
        ~EntFuncDoorT();

        void Think(float FrameTime, unsigned long ServerFrameNr);
        void OnTrigger(BaseEntityT* Activator);

        void Draw(bool FirstPersonView, float LodDist) const;


        const cf::TypeSys::TypeInfoT* GetType() const;
        static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
        static const cf::TypeSys::TypeInfoT TypeInfo;


        private:

        void DoDeserialize(cf::Network::InStreamT& Stream);     // Override the BaseEntityT base class method.

        void UpdateMovePos(float MoveFraction_);

        enum DoorStateT { Closed, Opening, Open, Closing };

        const cf::ClipSys::CollisionModelT* InfraredCollMdl;    ///< The collision model for the doors "infrared" trigger volume, covering both sides of the door (and the door itself).
        cf::ClipSys::ClipModelT             InfraredClipMdl;    ///< The related clip model for the automatic "infrared" operation of the door.

        DoorStateT DoorState;
        Vector3dT  OpenPos;             ///< The position of the door when open.
        Vector3dT  ClosedPos;           ///< The position of the door when closed.
        float      MoveTime;            ///< The time it takes the door to open or close.
        float      MoveFraction;        ///< 0.0 at the closed door position, 1.0 at the open door position.
        float      OpenTime;            ///< The time the door stays open before it automatically closes again.
        float      OpenTimeLeft;        ///< The time that is left until we're closing again.

        EntFuncDoorT* TeamHead;         ///< The head of a linked list of doors that are all in the same team.
        EntFuncDoorT* TeamNext;         ///< The next door in the linked list.

        const cf::SceneGraph::GenericNodeT* RootNode;   ///< The root node of the scene graph of the model (brushwork) of this entity.


        // Script methods (to be called from the map/entity Lua scripts).
        static int SetOrigin(lua_State* LuaState);  // Override for the base class method (we also have to update our ClipModel origin).

        static const luaL_Reg MethodsList[];
    };
}

#endif
