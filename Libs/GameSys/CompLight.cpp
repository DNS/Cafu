/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "CompLight.hpp"
#include "AllComponents.hpp"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace cf::GameSys;


const char* ComponentLightT::DocClass =
    "The common base class for light source components.";


ComponentLightT* ComponentLightT::Clone() const
{
    return new ComponentLightT(*this);
}


/***********************************/
/*** TypeSys-related definitions ***/
/***********************************/

void* ComponentLightT::CreateInstance(const cf::TypeSys::CreateParamsT& Params)
{
    return new ComponentLightT();
}

const cf::TypeSys::TypeInfoT ComponentLightT::TypeInfo(GetComponentTIM(), "GameSys::ComponentLightT", "GameSys::ComponentBaseT", ComponentLightT::CreateInstance, NULL, DocClass);
