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

#ifndef CAFU_CLIENT_WINDOW_HPP_INCLUDED
#define CAFU_CLIENT_WINDOW_HPP_INCLUDED

#include "GuiSys/Window.hpp"


class ClientT;


/// This class represents a window of the GuiSys into which the client renders the world.
class ClientWindowT : public cf::GuiSys::WindowT
{
    public:

    /// Creates a GuiSys window for the client Cl.
    ClientWindowT(const cf::GuiSys::WindowCreateParamsT& Params);

    void SetClient(ClientT* Cl);

    // Overloaded methods of the parent class.
    void Render() const;
    bool OnInputEvent(const CaKeyboardEventT& KE);
    bool OnInputEvent(const CaMouseEventT&    ME, float PosX, float PosY);
    bool OnClockTickEvent(float t);


    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    ClientT* Client;
    float    LastFrameTime;

    static const luaL_Reg MethodsList[];
};

#endif
