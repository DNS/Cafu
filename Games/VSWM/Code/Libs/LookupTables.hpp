/*******************************/
/*** Look-up Tables (Header) ***/
/*******************************/


#ifndef CAFU_LOOKUP_TABLES_HPP_INCLUDED
#define CAFU_LOOKUP_TABLES_HPP_INCLUDED


namespace LookupTables
{
    extern float HeadingToSin[2 << 16];
    extern float HeadingToCos[2 << 16];

    // Initialisiert die Look-up Tables. Sollte aus DllMain() aufgerufen werden!
    void Initialize();
};

#endif
