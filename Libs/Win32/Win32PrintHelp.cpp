/*
=================================================================================
This file is part of Cafu, the open-source game engine and graphics engine
for multiplayer, cross-platform, real-time 3D action.
Copyright (C) 2002-2012 Carsten Fuchs Software.

Cafu is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Cafu is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cafu. If not, see <http://www.gnu.org/licenses/>.

For support and more information about Cafu, visit us at <http://www.cafu.de>.
=================================================================================
*/

/****************************************/
/*** Print Help for Windows 32 (Code) ***/
/****************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "Win32PrintHelp.hpp"
#include "Templates/Array.hpp"

#if defined(_WIN32)
    #if defined(_MSC_VER)
        #define vsnprintf _vsnprintf
    #endif
#endif


const char* GetString(const char* String, ...)
{
    if (!String) return NULL;

    va_list     ArgList;
    static char FullString[1024];

    va_start(ArgList, String);
        vsnprintf(FullString, sizeof(FullString), String, ArgList);
    va_end(ArgList);

    FullString[sizeof(FullString)-1]=0;

    return FullString;
}


static ArrayT< ArrayT<char> > QueueStart;
static ArrayT< ArrayT<char> > QueueEnd;


void EnqueueString(const char* String, ...)
{
    if (!String) return;

    va_list     ArgList;
    static char FullString[1024];

    va_start(ArgList, String);
        vsnprintf(FullString, sizeof(FullString), String, ArgList);
    va_end(ArgList);

    FullString[sizeof(FullString)-1]=0;

    QueueEnd.PushBackEmpty();
    QueueEnd[QueueEnd.Size()-1].PushBackEmpty((unsigned long)(strlen(FullString)+1));
    strcpy(&QueueEnd[QueueEnd.Size()-1][0], FullString);
}


const char* DequeueString()
{
    if (QueueStart.Size()==0 && QueueEnd.Size()>0)
    {
        unsigned long Pos;

        for (Pos=QueueEnd.Size()-1; Pos>0; Pos--) QueueStart.PushBack(QueueEnd[Pos]);
        QueueStart.PushBack(QueueEnd[Pos]);

        QueueEnd.Clear();
    }

    if (QueueStart.Size()==0) return NULL;

    static char StringBuffer[1024];

    strcpy(StringBuffer, &QueueStart[QueueStart.Size()-1][0]);
    QueueStart.DeleteBack();

    return StringBuffer;
}
