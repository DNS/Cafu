/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
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
