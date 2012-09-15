/******************************/
/*** Info 2D Move Direction ***/
/******************************/

#include <cstdio>
#include <string.h>
#include "Info2DMoveDir.hpp"
#include "EntityCreateParams.hpp"

using namespace GAME_NAME;


EntInfo2DMoveDirT::EntInfo2DMoveDirT(char TypeID, unsigned long ID, unsigned long MapFileID, cf::GameSys::GameWorldI* GameWorld, const VectorT& Origin)
    : BaseEntityT(EntityCreateParamsT(ID, std::map<std::string, std::string>(), NULL, NULL, MapFileID, GameWorld, Origin),
                  BoundingBox3dT(Vector3dT( 100.0,  100.0,  100.0),
                                 Vector3dT(-100.0, -100.0, -100.0)),
                  0)
{
    // Werte die 'PropertyPairs' aus, die von der Basis-Klasse 'BaseEntity' noch nicht ausgewertet wurden!
    for (std::map<std::string, std::string>::const_iterator It=Properties.begin(); It!=Properties.end(); ++It)
    {
        const std::string& Key  =It->first;
        const std::string& Value=It->second;

        if (Key=="move_dir_name") MoveDirName=Value;
    }


    const VectorT MoveDir =m_Origin;
    const double  MoveDist=length(MoveDir);
    const VectorT MoveDir1=MoveDist>0.1 ? scale(MoveDir, 1.0/MoveDist) : VectorT(0.0, 1.0, 0.0);
    const double  Pi      =3.14159265359;
    const double  Hdg     =MoveDir1.x>=0.0 ? acos(MoveDir1.y) : 2.0*Pi-acos(MoveDir1.y);

    m_Heading=(unsigned short)(Hdg/Pi*32768.0);

    printf("EntInfo2DMoveDir: (%6.3f, %6.3f) %6.3f %5u  %s\n", MoveDir1.x, MoveDir1.y, Hdg, m_Heading, MoveDirName.c_str());
}
