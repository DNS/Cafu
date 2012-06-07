/******************************/
/*** Info 2D Move Direction ***/
/******************************/

#include <cstdio>
#include <string.h>
#include "Info2DMoveDir.hpp"
#include "EntityCreateParams.hpp"


EntInfo2DMoveDirT::EntInfo2DMoveDirT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin)
    : BaseEntityT(EntityCreateParamsT(ID, std::map<std::string, std::string>(), NULL, NULL, MapFileID, GameWorld, Origin),
                  0,
                  EntityStateT(Origin,
                               VectorT(),
                               BoundingBox3T<double>(VectorT( 100.0,  100.0,  100.0),
                                                     VectorT(-100.0, -100.0, -100.0)),
                               0,       // Heading (will be set when the BaseEntity evaluates the value of the "angles" property (in PropertyPairs))
                               0,
                               0,
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
    // Werte die 'PropertyPairs' aus, die von der Basis-Klasse 'BaseEntity' noch nicht ausgewertet wurden!
    for (std::map<std::string, std::string>::const_iterator It=Properties.begin(); It!=Properties.end(); ++It)
    {
        const std::string& Key  =It->first;
        const std::string& Value=It->second;

        if (Key=="move_dir_name") MoveDirName=Value;
    }


    const VectorT MoveDir =State.Origin;
    const double  MoveDist=length(MoveDir);
    const VectorT MoveDir1=MoveDist>0.1 ? scale(MoveDir, 1.0/MoveDist) : VectorT(0.0, 1.0, 0.0);
    const double  Pi      =3.14159265359;
    const double  Hdg     =MoveDir1.x>=0.0 ? acos(MoveDir1.y) : 2.0*Pi-acos(MoveDir1.y);

    State.Heading=(unsigned short)(Hdg/Pi*32768.0);

    printf("EntInfo2DMoveDir: (%6.3f, %6.3f) %6.3f %5u  %s\n", MoveDir1.x, MoveDir1.y, Hdg, State.Heading, MoveDirName.c_str());
}
