/********************************/
/*** Info Player Start (Code) ***/
/********************************/

#include <string.h>
#include "InfoPlayerStart.hpp"
#include "EntityCreateParams.hpp"


EntInfoPlayerStartT::EntInfoPlayerStartT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin)
    : BaseEntityT(EntityCreateParamsT(ID, std::map<std::string, std::string>(), NULL, NULL, MapFileID, GameWorld, Origin),
                  BoundingBox3dT(Vector3dT( 100.0,  100.0,  100.0),
                                 Vector3dT(-100.0, -100.0, -100.0)),
                  0,
                  EntityStateT(VectorT(),
                               0,
                               0,
                               0,       // ModelIndex
                               0,       // ModelSequNr
                               0.0,     // ModelFrameNr
                               0,       // Health
                               0,       // Armor
                               0,       // HaveItems
                               0,       // HaveWeapons
                               0,       // ActiveWeaponSlot
                               0,       // ActiveWeaponSequNr
                               0.0))    // ActiveWeaponFrameNr
{
}
