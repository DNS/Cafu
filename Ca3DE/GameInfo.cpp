/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "GameInfo.hpp"


GameInfoT::GameInfoT(const std::string& GameName)
    : m_GameName(GameName)
{
}


const std::string& GameInfoT::GetName() const
{
    return m_GameName;
}
