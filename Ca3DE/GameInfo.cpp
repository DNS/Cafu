/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "GameInfo.hpp"
#include "PlatformAux.hpp"

#include <stdexcept>


GameInfoT::GameInfoT(const std::string& GameName)
    : m_GameName(GameName)
{
}


const std::string& GameInfoT::GetName() const
{
    return m_GameName;
}


GameInfosT::GameInfosT()
{
    const std::vector<std::string> GameNames = PlatformAux::GetDirectory("Games", 'd');

    for (size_t i = 0; i < GameNames.size(); i++)
        m_AllGameInfos.push_back(GameInfoT(GameNames[i]));

    if (m_AllGameInfos.size() == 0)
        throw std::runtime_error("Could not find any game in the Games subdirectory.");

    m_GameInfo = m_AllGameInfos[0];
}
