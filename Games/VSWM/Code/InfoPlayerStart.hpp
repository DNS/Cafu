/**********************************/
/*** Info Player Start (Header) ***/
/**********************************/


#ifndef CAFU_INFOPLAYERSTART_HPP_INCLUDED
#define CAFU_INFOPLAYERSTART_HPP_INCLUDED


#include "BaseEntity.hpp"


namespace GAME_NAME
{
    class EntInfoPlayerStartT : public BaseEntityT
    {
        public:

        EntInfoPlayerStartT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin);
    };
}

#endif
