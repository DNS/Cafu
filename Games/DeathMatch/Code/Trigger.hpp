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

#ifndef CAFU_TRIGGER_HPP_INCLUDED
#define CAFU_TRIGGER_HPP_INCLUDED

#include "../../BaseEntity.hpp"


class EntityCreateParamsT;
struct luaL_Reg;
// namespace cf { namespace SceneGraph { class GenericNodeT; } }


/// This class implements "Trigger" entities.
///
/// For the user (mapper), it looks as if trigger entities call script methods whenever something walks into their (trigger) brushes,
/// but in truth, the roles are reversed, and it is e.g. the player movement code that checks if it walked itself into a trigger volume,
/// and then initiates the script function call. Trigger entities are therefore really quite passive, but thinking of them the other
/// way round (triggers are actively checking their volumes and activate the calls) is probably more suggestive to the users (mappers).
///
/// Note that the mapper is free to put trigger brushes also into any other entity of any entity class - it will work all the same.
/// However, the dedicated Trigger entity provides an "identity" to the trigger brushes (e.g. if you put all trigger brushes into the world,
/// you cannot distinguish them from each other and not address them or deal with them individually), plus it provides trigger-specific
/// behaviour by the overridden OnTrigger() method, plus trigger-specific script methods like Activate(), Deactivate() and IsActive().
class EntTriggerT : public BaseEntityT
{
    public:

    EntTriggerT(const EntityCreateParamsT& Params);
 // void Think(float FrameTime, unsigned long ServerFrameNr);
    void OnTrigger(BaseEntityT* Activator);


    const cf::TypeSys::TypeInfoT* GetType() const;
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

 // const cf::SceneGraph::GenericNodeT* RootNode;   ///< The root node of the scene graph of the model (brushwork) of this entity.
    bool IsActive;      ///< Whether this trigger is active or not. Inactive triggers just do nothing when their OnTrigger() method is called.


    // Script methods (to be called from the map/entity Lua scripts).
    static int Activate(lua_State* LuaState);
    static int Deactivate(lua_State* LuaState);
    static int GetIsActive(lua_State* LuaState);

    static const luaL_Reg MethodsList[];
};

#endif
