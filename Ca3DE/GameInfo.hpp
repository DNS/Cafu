/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_GAMEINFO_HPP_INCLUDED
#define CAFU_GAMEINFO_HPP_INCLUDED

#include <string>
#include <vector>


/// This class encapsulates information about a game.
class GameInfoT
{
    public:

    GameInfoT(const std::string& GameName="");

    const std::string& GetName() const;


    private:

    std::string m_GameName;
};


class GameInfosT
{
    public:

    GameInfosT();

    const GameInfoT& getCurrentGameInfo() const
    {
        return m_GameInfo;
    }

    bool setGame(const std::string& Name)
    {
        for (unsigned int i = 0; i < m_AllGameInfos.size(); i++)
            if (m_AllGameInfos[i].GetName() == Name)
            {
                m_GameInfo = m_AllGameInfos[i];
                return true;
            }

        return false;
    }

    std::string getList() const
    {
        std::string GamesList;

        for (unsigned int i = 0; i < m_AllGameInfos.size(); i++)
        {
            if (i > 0) GamesList += ", ";
            GamesList += m_AllGameInfos[i].GetName();
        }

        return GamesList;
    }


    private:

    std::vector<GameInfoT> m_AllGameInfos;  ///< The game infos for all games/MODs known and available to us.
    GameInfoT              m_GameInfo;      ///< The info of the game that was elected to run (one of those in m_AllGameInfos).
};

#endif
