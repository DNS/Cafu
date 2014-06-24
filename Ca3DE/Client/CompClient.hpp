/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2014 Carsten Fuchs Software.

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

#ifndef CAFU_GUISYS_COMPONENT_CLIENT_HPP_INCLUDED
#define CAFU_GUISYS_COMPONENT_CLIENT_HPP_INCLUDED

#include "GuiSys/CompBase.hpp"


class ClientT;


/// This component connects the Cafu game client to the GUI window that houses it.
class ComponentClientT : public cf::GuiSys::ComponentBaseT
{
    public:

    /// The constructor.
    ComponentClientT();

    /// The copy constructor.
    /// @param Comp   The component to create a copy of.
    ComponentClientT(const ComponentClientT& Comp);

    void SetClient(ClientT* Cl);

    // Base class overrides.
    ComponentClientT* Clone() const;
    const char* GetName() const { return "Client"; }
    void Render() const;
    bool OnInputEvent(const CaKeyboardEventT& KE);
    bool OnInputEvent(const CaMouseEventT& ME, float PosX, float PosY);
    void OnClockTickEvent(float t);

    // The TypeSys related declarations for this class.
    const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    // The Lua API methods of this class.
    static const luaL_Reg MethodsList[];        ///< The list of Lua methods for this class.
    static int toString(lua_State* LuaState);   ///< Returns a string representation of this object.

    ClientT* m_Client;
    float    m_LastFrameTime;
};

#endif
