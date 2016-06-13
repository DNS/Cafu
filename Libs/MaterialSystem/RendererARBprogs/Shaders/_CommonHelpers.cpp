/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/*************************/
/*** Common Cg Helpers ***/
/*************************/

#include "_CommonHelpers.hpp"
#include "../../Common/OpenGLEx.hpp"
#include "ConsoleCommands/Console.hpp"
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
    cf::glProgramStringARB(ProgramTarget, GL_PROGRAM_FORMAT_ASCII_ARB, GLsizei(strlen(ProgramCode)), ProgramCode);


    // Error checking.
    GLenum      ErrorID    =glGetError();
    const char* ErrorString=(const char*)glGetString(GL_PROGRAM_ERROR_STRING_ARB);

    GLint ErrorPos;
    glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &ErrorPos);

#ifdef DEBUG
    // Want to see warning messages in debug builts.
    const size_t ErrorStringLen=strlen(ErrorString);
#else
    // ...and currently in release builts, too.
    const size_t ErrorStringLen=strlen(ErrorString);
 // const int ErrorStringLen=0;
#endif

    if (ErrorID!=GL_NO_ERROR || ErrorPos!=-1 || ErrorStringLen>0)
    {
        Console->Print(cf::va("%s\n\nProblem detected:\nglGetError() == %u,\nerror position: %i,\nerror string: %s\n", ProgramCode, ErrorID, ErrorPos, ErrorString));

        FILE* ErrorFile=fopen("ProgError.txt", "a");

        if (ErrorFile)
        {
            fprintf(ErrorFile, "%s\n\nProblem detected:\nglGetError() == %u,\nerror position: %i,\nerror string: %s\n", ProgramCode, ErrorID, ErrorPos, ErrorString);
            fclose(ErrorFile);
        }
    }

    return ProgramName;
}
