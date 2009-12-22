/*******************************/
/*** Look-up Tables (Header) ***/
/*******************************/


#ifndef _LOOKUP_TABLES_HPP_
#define _LOOKUP_TABLES_HPP_


namespace LookupTables
{
    extern float HeadingToSin[2 << 16];
    extern float HeadingToCos[2 << 16];

    // Initialisiert die Look-up Tables. Sollte aus DllMain() aufgerufen werden!
    void Initialize();
};

#endif
