/**********************************/
/*** Info Player Start (Header) ***/
/**********************************/


#ifndef _INFOPLAYERSTART_HPP_
#define _INFOPLAYERSTART_HPP_


#include "../../BaseEntity.hpp"


class EntInfoPlayerStartT : public BaseEntityT
{
    public:

    EntInfoPlayerStartT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin);
};

#endif
