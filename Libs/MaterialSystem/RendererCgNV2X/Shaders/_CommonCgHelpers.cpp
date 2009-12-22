/*
=================================================================================
This file is part of Cafu, the open-source game and graphics engine for
multiplayer, cross-platform, real-time 3D action.
$Id$

Copyright (C) 2002-2010 Carsten Fuchs Software.

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

/*************************/
/*** Common Cg Helpers ***/
/*************************/

#include "_CommonCgHelpers.hpp"
#include <stdio.h>


CGprogram UploadCgProgram(CGcontext CgContext, CGprofile Profile, const char* SourceCode, bool IsObjectCode)
{
    const char* CgCompilerOptions[3]={ "-strict", "-fastprecision", NULL };

    CGprogram NewCgProgram=cgCreateProgram(CgContext, IsObjectCode ? CG_OBJECT : CG_SOURCE, SourceCode, Profile, NULL, CgCompilerOptions);

    if (NewCgProgram==NULL)
    {
        FILE*   ErrorFile=fopen("CgError.txt", "a");
        CGerror err      =cgGetError();

        printf("ERROR: Unable to create Cg program: %s\n", cgGetErrorString(err));
        if (ErrorFile) fprintf(ErrorFile, "ERROR: Unable to create Cg program: %s\n", cgGetErrorString(err));

        const char* LastListing=cgGetLastListing(CgContext);
        if (LastListing) printf("LAST LISTING:\n%s\n", LastListing);
        if (LastListing && ErrorFile) fprintf(ErrorFile, "LAST LISTING:\n%s\n", LastListing);

        if (ErrorFile) fclose(ErrorFile);
        return NULL;
    }

    cgGLLoadProgram(NewCgProgram);

    CGerror err=cgGetError();
    if (err!=CG_NO_ERROR)
    {
        // This should never happen, as we have already made sure elsewhere that the desired profile is supported!
        printf("ERROR: Unable to load Cg program: %s\n", cgGetErrorString(err));
        return NULL;
    }

    return NewCgProgram;
}
