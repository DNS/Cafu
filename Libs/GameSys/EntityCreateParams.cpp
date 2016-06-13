/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "EntityCreateParams.hpp"

#include <climits>


using namespace cf::GameSys;


EntityCreateParamsT::EntityCreateParamsT(WorldT& World_)
    : World(World_),
      m_ID(UINT_MAX)
{
}


void EntityCreateParamsT::ForceID(unsigned int ID)
{
    m_ID = ID;
}
