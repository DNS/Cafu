/******************************/
/*** Info 2D Move Direction ***/
/******************************/

#ifndef CAFU_INFO2DMOVEDIRECTION_HPP_INCLUDED
#define CAFU_INFO2DMOVEDIRECTION_HPP_INCLUDED

#include "BaseEntity.hpp"


namespace GAME_NAME
{
    class EntInfo2DMoveDirT : public BaseEntityT
    {
        public:

        EntInfo2DMoveDirT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin);

        std::string MoveDirName;
    };
}

#endif
