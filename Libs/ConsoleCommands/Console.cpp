/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "Console.hpp"
#include <stdio.h>
#include <stdarg.h>

#if defined(_WIN32) && defined (_MSC_VER)
    #if (_MSC_VER<1300)
        #define vsnprintf _vsnprintf
        #define for if (false) ; else for
    #endif
#endif


std::string cf::va(const char* FormatString, ...)
{
    va_list ArgList;
    char    Buffer[1024];

    if (!FormatString) return "";

    va_start(ArgList, FormatString);
    vsnprintf(Buffer, 1024-1, FormatString, ArgList);
    Buffer[1024-1]=0;
    va_end(ArgList);

    return Buffer;
}
