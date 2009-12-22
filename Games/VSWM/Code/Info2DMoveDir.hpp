/******************************/
/*** Info 2D Move Direction ***/
/******************************/

#ifndef _INFO2DMOVEDIRECTION_HPP_
#define _INFO2DMOVEDIRECTION_HPP_

#include "../../BaseEntity.hpp"


class EntInfo2DMoveDirT : public BaseEntityT
{
    public:

    EntInfo2DMoveDirT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin);

    std::string MoveDirName;
};

#endif
