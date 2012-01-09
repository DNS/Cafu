/*************************/
/*** Info Node Spacing ***/
/*************************/

#ifndef CAFU_INFONODESPACING_HPP_INCLUDED
#define CAFU_INFONODESPACING_HPP_INCLUDED

#include "../../BaseEntity.hpp"


class EntInfoNodeSpacingT : public BaseEntityT
{
    public:

    EntInfoNodeSpacingT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin);
};

#endif
