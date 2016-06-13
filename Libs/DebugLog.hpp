/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_DEBUG_LOG_HPP_INCLUDED
#define CAFU_DEBUG_LOG_HPP_INCLUDED


namespace cf
{
    void LogDebugBasic(const char* Category, const char* FileName, const int LineNr, const char* FormatString, ...);

    inline void LogDebugNOP() { }
}


// Enable or disable debug logging here.
// Note that filters (which categories are included in or exempted from logging)
// are defined in the implementation of LogDebugBasic().
#if 0
    #define LogDebug(cat, fmt, ...)   LogDebugBasic(#cat, __FILE__, __LINE__, fmt, __VA_ARGS__)
#else
    #define LogDebug(cat, fmt, ...)   LogDebugNOP()
#endif

#endif
