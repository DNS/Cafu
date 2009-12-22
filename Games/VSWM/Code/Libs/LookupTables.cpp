/*****************************/
/*** Look-up Tables (Code) ***/
/*****************************/


#include <math.h>
#include "LookupTables.hpp"


float LookupTables::HeadingToSin[2 << 16];
float LookupTables::HeadingToCos[2 << 16];


void LookupTables::Initialize()
{
    const double Pi=3.14159265359;

    for (unsigned long Heading=0; Heading<(2 << 16); Heading++)
    {
        HeadingToSin[Heading]=float(sin(double(Heading)/32768.0*Pi));
        HeadingToCos[Heading]=float(cos(double(Heading)/32768.0*Pi));
    }
}
