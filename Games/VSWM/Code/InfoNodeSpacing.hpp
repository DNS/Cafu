/*************************/
/*** Info Node Spacing ***/
/*************************/

#ifndef _INFONODESPACING_HPP_
#define _INFONODESPACING_HPP_

#include "../../BaseEntity.hpp"


class EntInfoNodeSpacingT : public BaseEntityT
{
    public:

    EntInfoNodeSpacingT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin);
};

#endif
