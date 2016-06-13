/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMEINFO_HPP_INCLUDED
#define CAFU_GAMEINFO_HPP_INCLUDED

#include <string>


/// This class encapsulates information about a game.
class GameInfoT
{
    public:

    GameInfoT(const std::string& GameName="");

    const std::string& GetName() const;


    private:

    std::string m_GameName;
};

#endif
