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

#include "_CommonHelpers.hpp"
#include "../../Common/OpenGLEx.hpp"
#include <stdio.h>
#include <string.h>


GLuint UploadProgram(GLenum ProgramTarget, const char* ProgramCode)
{
    GLuint ProgramName;

    // Generate a program object handle.
    cf::glGenProgramsARB(1, &ProgramName);

    // Make ProgramName the current program object (and thereby create it).
    cf::glBindProgramARB(ProgramTarget, ProgramName);

    // Upload the string (code) for this program.
    cf::glProgramStringARB(ProgramTarget, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(ProgramCode), ProgramCode);


    // Error checking.
    GLenum      ErrorID    =glGetError();
    const char* ErrorString=(const char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);

    GLint ErrorPos;
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &ErrorPos);

#ifdef DEBUG
    // Want to see warning messages in debug builts.
    const int ErrorStringLen=strlen(ErrorString);
#else
    // ...and currently in release builts, too.
    const int ErrorStringLen=strlen(ErrorString);
 // const int ErrorStringLen=0;
#endif

    if (ErrorID!=GL_NO_ERROR || ErrorPos!=-1 || ErrorStringLen>0)
    {
        printf("%s\n\nProblem detected:\nglGetError() == %i,\nerror position: %i,\nerror string: %s\n", ProgramCode, ErrorID, ErrorPos, ErrorString);

        FILE* ErrorFile=fopen("ProgError.txt", "a");

        if (ErrorFile)
        {
            fprintf(ErrorFile, "%s\n\nProblem detected:\nglGetError() == %i,\nerror position: %i,\nerror string: %s\n", ProgramCode, ErrorID, ErrorPos, ErrorString);
            fclose(ErrorFile);
        }
    }

    return ProgramName;
}
